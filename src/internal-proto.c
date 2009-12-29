#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "res-conn-private.h"
#include "res-set-private.h"
#include "internal-msg.h"
#include "internal-proto.h"

typedef struct {
    char      name[64];
    uint32_t  serial;
    int32_t   errcod;
    char      errmsg[512];
} statuscb_data_t;

static resconn_internal_t   *resproto_manager;
static uint32_t              timeout = 2000;

static resset_t *connect_to_manager(resconn_t *, resmsg_t *);
static resset_t *connect_fail(resconn_t *, resmsg_t *);

static int  send_message(resset_t *, resmsg_t *, resproto_status_t);
static int  send_error_init(resset_t *, resmsg_t *, void *);
static int  send_error_complete(void *);
static void receive_message_init(resconn_internal_t *, char *,
                                 uint32_t, resmsg_t *);
static int  receive_message_dequeue(void *);
static void receive_message_complete(resconn_internal_t *, resconn_qitem_t *);

static resconn_internal_t *find_resconn_client(resset_t *);

static int queue_is_empty(resconn_qhead_t *);
static void queue_append_item(resconn_qhead_t *, resconn_qitem_t *);
static resconn_qitem_t *queue_pop_item(resconn_qhead_t *);


int resproto_internal_manager_init(resconn_internal_t *rcon, va_list args)
{
    resconn_timer_add_t  timer_add = va_arg(args, resconn_timer_add_t);
    resconn_timer_del_t  timer_del = va_arg(args, resconn_timer_del_t);
    int                  success;

    if (resproto_manager != NULL)
        success = (rcon == resproto_manager);
    else {
        success = TRUE;

        rcon->connect   = connect_fail;
        rcon->disconn   = resset_destroy;
        rcon->send      = send_message;
        rcon->error     = send_error_init;
        rcon->name      = strdup(RESPROTO_INTERNAL_MANAGER);
        rcon->queue.head.next = (void *)&rcon->queue.head;
        rcon->queue.head.prev = (void *)&rcon->queue.head;
        rcon->timer.add = timer_add;
        rcon->timer.del = timer_del;

        resproto_manager = rcon;
    }
  
    return success;
}

int resproto_internal_client_init(resconn_internal_t *rcon, va_list args)
{
    resconn_linkup_t     mgrup     = va_arg(args, resconn_linkup_t);
    char                *name      = va_arg(args, char *);
    resconn_timer_add_t  timer_add = va_arg(args, resconn_timer_add_t);
    resconn_timer_del_t  timer_del = va_arg(args, resconn_timer_del_t);
    int                  success   = TRUE;

    rcon->connect   = connect_to_manager;
    rcon->disconn   = resset_destroy;
    rcon->send      = send_message;
    rcon->error     = send_error_init;
    rcon->mgrup     = mgrup;
    rcon->name      = strdup(name);
    rcon->queue.head.next = (void *)&rcon->queue.head;
    rcon->queue.head.prev = (void *)&rcon->queue.head;
    rcon->timer.add = timer_add;
    rcon->timer.del = timer_del;
  
    return success;
}

static resset_t *connect_to_manager(resconn_t *rcon, resmsg_t *resmsg)
{
    char          *name  =  RESPROTO_INTERNAL_MANAGER;
    uint32_t       id    =  resmsg->any.id;
    resmsg_rset_t *flags = &resmsg->record.rset;
    const char    *class =  resmsg->record.class;
    resset_t      *rset;

    if ((rset = resset_find(rcon, name, id)) == NULL)
        rset = resset_create(rcon, name, id, RESPROTO_RSET_STATE_CREATED,
                             class, flags->all, flags->share, flags->opt);

    return rset;
}

static resset_t *connect_fail(resconn_t *rcon, resmsg_t *resmsg)
{
    (void)rcon;
    (void)resmsg;

    return NULL;
}


static int send_message(resset_t          *rset,
                        resmsg_t          *resmsg,
                        resproto_status_t  status)
{
    static uint32_t     reply_id = 1;

    resconn_internal_t *rcon;
    resconn_internal_t *receiver;
    int                 need_reply;
    resmsg_type_t       type;
    uint32_t            serial;
    uint32_t            reqno;
    resconn_reply_t    *reply;
    statuscb_data_t    *std;
    int                 success;

    if (!rset || !resmsg)
        return FALSE;

    rcon = &rset->resconn->internal;

    switch (rcon->role) {
    case RESPROTO_ROLE_MANAGER:  receiver = find_resconn_client(rset);   break;
    case RESPROTO_ROLE_CLIENT:   receiver = resproto_manager;            break;
    default:                     receiver = NULL;                        break;
    }

    if (!receiver)
        success = FALSE;
    else {
        success = TRUE;

        if (rcon->role != RESPROTO_ROLE_CLIENT)
            need_reply = status ? TRUE : FALSE;
        else {
            switch (resmsg->any.type) {
            case RESMSG_REGISTER:    need_reply = TRUE;                  break;
            case RESMSG_UNREGISTER:  need_reply = TRUE;                  break;
            default:                 need_reply = status ? TRUE : FALSE; break;
            }
        }

        if (!need_reply)
            serial = 0;
        else {
            type   = resmsg->type;
            serial = reply_id++;
            reqno  = resmsg->any.reqno;
            reply  = resconn_reply_create(type, serial, reqno, rset, status);

            if ((std = malloc(sizeof(statuscb_data_t))) != NULL) {
                memset(std, 0, sizeof(statuscb_data_t));
                strncpy(std->name, rcon->name, sizeof(std->name)-1);
                strncpy(std->errmsg, "Internal.NoReply",sizeof(std->errmsg)-1);
                std->serial = serial;
                std->errcod = ETIME;

                reply->data  = std;
                reply->timer = rcon->timer.add(timeout,
                                               send_error_complete,
                                               std);
            }
        }

        rcon->busy = TRUE;

        receive_message_init(receiver, rcon->name, serial, resmsg);

        rcon->busy = FALSE;
    }
    
    return success;
}

static int send_error_init(resset_t *rset, resmsg_t *resreply, void *data)
{
    resconn_internal_t *rcon = &rset->resconn->internal;
    statuscb_data_t    *std;
    int                 success;

    if ((std = malloc(sizeof(statuscb_data_t))) == NULL)
        success = FALSE;
    else {
        memset(std, 0, sizeof(statuscb_data_t));
        strncpy(std->name, rset->peer, sizeof(std->name)-1);
        std->serial = (uint32_t)data;
        std->errcod = resreply->status.errcod;
        
        if (resreply->status.errmsg)
            strncpy(std->errmsg,resreply->status.errmsg,sizeof(std->errmsg)-1);

        rcon->timer.add(0, send_error_complete, std);

        success = TRUE;
    }

    return success;
}

static int send_error_complete(void *data)
{
    statuscb_data_t *std = (statuscb_data_t *)data;
    resconn_t       *rcon  = NULL;
    resconn_reply_t *reply;
    resset_t        *rset;
    resmsg_t         resmsg;
    
    while ((rcon = resconn_list_iterate(rcon)) != NULL) {
        if (rcon->any.transp == RESPROTO_TRANSPORT_INTERNAL &&
            !strcmp(rcon->internal.name, std->name)            )
        {
            if ((reply = resconn_reply_find(rcon, std->serial)) != NULL) {
                rset = reply->rset;

                if (reply->timer && reply->data != data) {
                    rcon->internal.timer.del(reply->timer);
                    reply->timer = NULL;
                    free(reply->data);
                }

                if (rcon->any.role == RESPROTO_ROLE_CLIENT) {
                    switch (reply->type) {

                    case RESMSG_REGISTER:
                        if (!std->errcod)
                            rset->state = RESPROTO_RSET_STATE_CONNECTED;
                        else
                            rset->state = RESPROTO_RSET_STATE_KILLED;
                        break;

                    case RESMSG_UNREGISTER:
                        if (resmsg.status.errcod) {
                            resset_ref(rset);
                            rset->state = RESPROTO_RSET_STATE_CONNECTED;
                        }
                        break;

                    default:
                        break;
                    }
                }

                if (reply->callback) {
                    resmsg.status.type   = RESMSG_STATUS;
                    resmsg.status.id     = rset->id;
                    resmsg.status.reqno  = reply->reqno;
                    resmsg.status.errcod = std->errcod;
                    resmsg.status.errmsg = std->errmsg;
                    
                    reply->callback(rset, &resmsg);
                }

                resconn_reply_destroy(reply);
            }

            break;
        }
    }

    free(std);

    return FALSE;
}


static void receive_message_init(resconn_internal_t *rcon,
                                 char               *peer,
                                 uint32_t            serial,
                                 resmsg_t           *msg)
{
    void            *data;
    int              queue_was_empty;
    resconn_qitem_t  buf;
    resconn_qitem_t *item;
    resmsg_rset_t   *flags;
    const char      *class;
    


    data  = (void *)serial;
    queue_was_empty = queue_is_empty(&rcon->queue.head);

    if (msg->type == RESMSG_REGISTER) {
        class =  msg->record.class;
        flags = &msg->record.rset;

        if ((resset_find((resconn_t *)rcon, peer, msg->any.id)) == NULL) {
            resset_create((resconn_t *)rcon, peer, msg->any.id,
                          RESPROTO_RSET_STATE_CONNECTED,
                          class, flags->all, flags->share, flags->opt);
        }
    }

    if (rcon->busy || !queue_was_empty) {
        if ((item = malloc(sizeof(resconn_qitem_t))) != NULL) {
            memset(item, 0, sizeof(resconn_qitem_t));
            item->peer = strdup(peer);
            item->data = data;
            item->msg = resmsg_internal_copy_message(msg);

            queue_append_item(&rcon->queue.head, item);

            rcon->queue.timer = rcon->timer.add(0,
                                                receive_message_dequeue,
                                                rcon);
        }
    }
    else {
        item = &buf;

        memset(item, 0, sizeof(resconn_qitem_t));
        item->peer = peer;
        item->data = data;
        item->msg  = msg;

        receive_message_complete(rcon, item);
    }
}

static int receive_message_dequeue(void *data)
{
    resconn_internal_t *rcon = (resconn_internal_t *)data;
    resconn_qitem_t    *item;

    if ((item = queue_pop_item(&rcon->queue.head)) != NULL) {

        receive_message_complete(rcon, item);

        free(item->peer);
        resmsg_internal_destroy_message(item->msg);
        free(item);
    }

    return FALSE;
}

static void receive_message_complete(resconn_internal_t *rcon,
                                     resconn_qitem_t    *item)
{
    resmsg_t *msg  = item->msg;
    resset_t *rset = resset_find((resconn_t *)rcon, item->peer, msg->any.id);
    
    if (rset != NULL){
        rcon->receive(msg, rset, item->data);
    }    
}


static resconn_internal_t *find_resconn_client(resset_t *rset)
{
    resconn_t *rcon = NULL;

    while ((rcon = resconn_list_iterate(rcon)) != NULL) {
        if (rcon->any.role   == RESPROTO_ROLE_CLIENT        &&
            rcon->any.transp == RESPROTO_TRANSPORT_INTERNAL &&
            !strcmp(rcon->internal.name, rset->peer)          )
        {
            break;
        }
    }
    
    return &rcon->internal;
}

static int queue_is_empty(resconn_qhead_t *queue)
{
    return (queue->next == (resconn_qitem_t *)queue) ? TRUE : FALSE;
}

static void queue_append_item(resconn_qhead_t *queue, resconn_qitem_t *item)
{
    resconn_qitem_t *prev;
    resconn_qitem_t *next;

    if (queue && item) {
        prev = queue->prev;
        next = prev->next;
        
        item->next = next;
        item->prev = prev;
        
        prev->next = item;
        next->prev = item;
    }
}

static resconn_qitem_t *queue_pop_item(resconn_qhead_t *queue)
{
    resconn_qitem_t *item = NULL;
    resconn_qitem_t *prev;
    resconn_qitem_t *next;

    if (queue != NULL && !queue_is_empty(queue)) {
        item = queue->next;
        prev = item->prev;
        next = item->next;

        prev->next = item->next;
        next->prev = item->prev;
    }

    return item;
}


/* 
 * Local Variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim:set expandtab shiftwidth=4:
 */
