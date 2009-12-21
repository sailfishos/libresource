#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "internal.h"

typedef struct {
    char      name[64];
    uint32_t  serial;
    int32_t   errcod;
    char      errmsg[512];
} statuscb_data_t;

static resproto_internal_t  *resproto_manager;
static uint32_t              timeout = 2000;

static resset_t *connect_to_manager(resproto_t *, resmsg_t *);
static resset_t *connect_fail(resproto_t *, resmsg_t *);

static int  send_message(resset_t *, resmsg_t *, resproto_status_t);
static int  send_error_init(resset_t *, resmsg_t *, void *);
static int  send_error_complete(void *);
static void receive_message(resproto_internal_t *, resproto_internal_t *,
                            uint32_t, resmsg_t *);

static resproto_internal_t *find_resproto_client(resset_t *);

int resproto_internal_manager_init(resproto_internal_t *rp, va_list args)
{
    resproto_timer_add_t  timer_add = va_arg(args, resproto_timer_add_t);
    resproto_timer_del_t  timer_del = va_arg(args, resproto_timer_del_t);
    int                   success;

    if (resproto_manager != NULL)
        success  = (rp == resproto_manager);
    else {
        success  = TRUE;

        rp->connect   = connect_fail;
        rp->disconn   = resset_destroy;
        rp->send      = send_message;
        rp->error     = send_error_init;
        rp->name      = strdup(RESPROTO_INTERNAL_MANAGER);
        rp->timer.add = timer_add;
        rp->timer.del = timer_del;

        resproto_manager = rp;
    }
  
    return success;
}

int resproto_internal_client_init(resproto_internal_t *rp, va_list args)
{
    resproto_linkup_t     mgrup     = va_arg(args, resproto_linkup_t);
    char                 *name      = va_arg(args, char *);
    resproto_timer_add_t  timer_add = va_arg(args, resproto_timer_add_t);
    resproto_timer_del_t  timer_del = va_arg(args, resproto_timer_del_t);
    int                   success = TRUE;

    rp->connect   = connect_to_manager;
    rp->disconn   = resset_destroy;
    rp->send      = send_message;
    rp->error     = send_error_init;
    rp->mgrup     = mgrup;
    rp->name      = strdup(name);
    rp->timer.add = timer_add;
    rp->timer.del = timer_del;
  
    return success;
}

static resset_t *connect_to_manager(resproto_t *rp, resmsg_t *resmsg)
{
    char     *name = RESPROTO_INTERNAL_MANAGER;
    uint32_t  id   = resmsg->any.id;
    resset_t *rset;

    if ((rset = resset_find(rp, name, id)) == NULL)
        rset = resset_create(rp, name, id, RESPROTO_RSET_STATE_CREATED);

    return rset;
}

static resset_t *connect_fail(resproto_t *rp, resmsg_t *resmsg)
{
    (void)rp;
    (void)resmsg;

    return NULL;
}


static int send_message(resset_t          *rset,
                        resmsg_t          *resmsg,
                        resproto_status_t  status)
{
    static uint32_t       reply_id = 1;

    resproto_internal_t  *rp;
    resproto_internal_t  *receiver;
    int                   need_reply;
    resmsg_type_t         type;
    uint32_t              serial;
    uint32_t              reqno;
    resproto_reply_t     *reply;
    statuscb_data_t      *std;
    int                   success;

    if (!rset || !resmsg)
        return FALSE;

    rp = &rset->resproto->internal;

    switch (rp->role) {
    case RESPROTO_ROLE_MANAGER:  receiver = find_resproto_client(rset);  break;
    case RESPROTO_ROLE_CLIENT:   receiver = resproto_manager;            break;
    default:                     receiver = NULL;                        break;
    }

    if (!receiver)
        success = FALSE;
    else {
        success = TRUE;

        if (rp->role != RESPROTO_ROLE_CLIENT)
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
            reply  = resproto_reply_create(type, serial, reqno, rset, status);

            if ((std = malloc(sizeof(statuscb_data_t))) != NULL) {
                memset(std, 0, sizeof(statuscb_data_t));
                strncpy(std->name, rp->name, sizeof(std->name)-1);
                strncpy(std->errmsg, "Internal.NoReply",sizeof(std->errmsg)-1);
                std->serial = serial;
                std->errcod = ETIME;

                reply->data  = std;
                reply->timer = rp->timer.add(timeout, send_error_complete,std);
            }
        }

        receive_message(receiver, rp, serial, resmsg);
    }
    
    return success;
}

static int send_error_init(resset_t *rset, resmsg_t *resreply, void *data)
{
    resproto_internal_t *rp = &rset->resproto->internal;
    statuscb_data_t     *std;
    int                  success;

    if ((std = malloc(sizeof(statuscb_data_t))) == NULL)
        success = FALSE;
    else {
        memset(std, 0, sizeof(statuscb_data_t));
        strncpy(std->name, rset->peer, sizeof(std->name)-1);
        std->serial = (uint32_t)data;
        std->errcod = resreply->status.errcod;
        
        if (resreply->status.errmsg)
            strncpy(std->errmsg,resreply->status.errmsg,sizeof(std->errmsg)-1);

        rp->timer.add(0, send_error_complete,std);

        success = TRUE;
    }

    return success;
}

static int send_error_complete(void *data)
{
    statuscb_data_t  *std = (statuscb_data_t *)data;
    resproto_t       *rp  = NULL;
    resproto_reply_t *reply;
    resset_t         *rset;
    resmsg_t          resmsg;
    
    while ((rp = resproto_list_iterate(rp)) != NULL) {
        if (rp->any.transp == RESPROTO_TRANSPORT_INTERNAL &&
            !strcmp(rp->internal.name, std->name)            )
        {
            if ((reply = resproto_reply_find(rp, std->serial)) != NULL) {
                rset = reply->rset;

                if (reply->timer && reply->data != data) {
                    rp->internal.timer.del(reply->timer);
                    reply->timer = NULL;
                    free(reply->data);
                }

                if (rp->any.role == RESPROTO_ROLE_CLIENT) {
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
            }

            break;
        }
    }

    free(std);

    return FALSE;
}


static void receive_message(resproto_internal_t *rp,
                            resproto_internal_t *sender,
                            uint32_t             serial,
                            resmsg_t            *resmsg)
{
    resset_t *rset;

    if (!(rset = resset_find((resproto_t*)rp, sender->name, resmsg->any.id))) {
        if (resmsg->type == RESMSG_REGISTER) {
            rset = resset_create((resproto_t*)rp, sender->name, resmsg->any.id,
                                 RESPROTO_RSET_STATE_CONNECTED);
        }
    }

    if (rset)
        rp->receive(resmsg, rset, (void *)serial);
}


static resproto_internal_t *find_resproto_client(resset_t *rset)
{
    resproto_t *rp = NULL;

    while ((rp = resproto_list_iterate(rp)) != NULL) {
        if (rp->any.role   == RESPROTO_ROLE_CLIENT        &&
            rp->any.transp == RESPROTO_TRANSPORT_INTERNAL &&
            !strcmp(rp->internal.name, rset->peer)          )
        {
            break;
        }
    }
    
    return &rp->internal;
}



/* 
 * Local Variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim:set expandtab shiftwidth=4:
 */
