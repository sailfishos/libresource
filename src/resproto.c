#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "resproto.h"
#include "dbus.h"
#include "internal.h"


static resproto_t    *resproto_list;
static uint32_t       internal_reqno;

#define VALID   1
#define INVALID 0

static int manager_valid_message[RESMSG_MAX] = {
    [RESMSG_REGISTER  ] = VALID   ,
    [RESMSG_UNREGISTER] = VALID   ,
    [RESMSG_UPDATE    ] = VALID   ,
    [RESMSG_ACQUIRE   ] = VALID   ,
    [RESMSG_RELEASE   ] = VALID   ,
    [RESMSG_GRANT     ] = INVALID ,
    [RESMSG_ADVICE    ] = INVALID ,
};
static int client_valid_message[RESMSG_MAX] = {
    [RESMSG_REGISTER  ] = INVALID ,
    [RESMSG_UNREGISTER] = VALID   ,
    [RESMSG_UPDATE    ] = INVALID ,
    [RESMSG_ACQUIRE   ] = INVALID ,
    [RESMSG_RELEASE   ] = INVALID ,
    [RESMSG_GRANT     ] = VALID   ,
    [RESMSG_ADVICE    ] = VALID   ,
};

#undef INVALID
#undef VALID


static void manager_link_handler(resproto_t *, resproto_linkst_t);
static void client_link_handler(resproto_t *, resproto_linkst_t);

static void message_receive(resmsg_t *, resset_t *, void *);

static void        resproto_list_add(resproto_t *);
static void        resproto_list_delete(resproto_t *);

resproto_t *resproto_init(resproto_role_t       role,
                          resproto_transport_t  transp,
                          ...  /* role & transport specific args */ )
{
    static uint32_t      id;

    va_list              args;
    resproto_t          *rp;
    int                  success;

    if ((rp = malloc(sizeof(resproto_t))) != NULL) {

        memset(rp, 0, sizeof(resproto_t));
        rp->any.id      = ++id;
        rp->any.role    = role;
        rp->any.transp  = transp;
        rp->any.receive = message_receive;

        va_start(args, transp);
        
        switch (role) {

        case RESPROTO_ROLE_MANAGER:
            rp->any.link    = manager_link_handler;
            rp->any.valid   = manager_valid_message;

            switch (transp) {
            case RESPROTO_TRANSPORT_DBUS:
                success = resproto_dbus_manager_init(&rp->dbus, args);
                break;
            case RESPROTO_TRANSPORT_INTERNAL:
                success = resproto_internal_manager_init(&rp->internal, args);
                break;
            default:
                success = FALSE;
            }

            break;
            
        case RESPROTO_ROLE_CLIENT:
            rp->any.link    = client_link_handler;
            rp->any.valid   = client_valid_message;

            switch (transp) {
            case RESPROTO_TRANSPORT_DBUS:
                success = resproto_dbus_client_init(&rp->dbus, args);
                break;
            case RESPROTO_TRANSPORT_INTERNAL:
                success = resproto_internal_client_init(&rp->internal, args);
                break;
            default:
                success = FALSE;
            }

            break;
            
        default:
            success = FALSE;
            break;
        }

        va_end(args);

        if (!success)
            goto failed;

        resproto_list_add(rp);
    }

    return rp;

 failed:
    free(rp);
    return NULL;
}


int resproto_set_handler(resproto_t          *rp,
                         resmsg_type_t        type,
                         resproto_handler_t   handler)
{
    if (type < 0 || type >= RESMSG_MAX || !rp->any.valid[type] || !handler)
        return FALSE;

   rp->any.handler[type] = handler;

    return TRUE;
}

resset_t *resproto_connect(resproto_t        *rp,
                           resmsg_t          *resmsg,
                           resproto_status_t  status)
{
    resset_t *rset;

    if (rp == NULL || rp->any.killed ||
        rp->any.role != RESPROTO_ROLE_CLIENT ||
        resmsg->type != RESMSG_REGISTER)
    {
        rset =  NULL;
    }
    else {
        rset = rp->any.connect(rp, resmsg);
        rp->any.send(rset, resmsg, status);
    }

    return rset;
}

int resproto_disconnect(resset_t          *rset,
                        resmsg_t          *resmsg,
                        resproto_status_t  status)
{
    resproto_t *rp = rset->resproto;
    int         success;

    if (rset         == NULL                          ||
        rset->state  != RESPROTO_RSET_STATE_CONNECTED ||
        resmsg->type != RESMSG_UNREGISTER               )
    {
        success = FALSE;
    }
    else {
        if ((success = rp->any.send(rset, resmsg, status)))
            rp->any.disconn(rset);
    }

    return success;
}

int resproto_send_message(resset_t          *rset,
                          resmsg_t          *resmsg,
                          resproto_status_t  status)
{
    resproto_t      *rp   = rset->resproto;
    resproto_role_t  role = rp->any.role;
    resmsg_type_t    type = resmsg->type;
    int              success;

    if (rset->state != RESPROTO_RSET_STATE_CONNECTED ||
        type == RESMSG_REGISTER || type == RESMSG_UNREGISTER)
        success = FALSE;
    else
        success = rp->any.send(rset, resmsg, status);

    return success;
}

int resproto_reply_message(resset_t   *rset,
                           resmsg_t   *resmsg,
                           void       *protodata,
                           int32_t     errcod,
                           const char *errmsg)
{
    resproto_t *rp = rset->resproto;
    resmsg_t    reply;
    int         success;

    if (!rset || !resmsg)
        success = FALSE;
    else {
        if (protodata == NULL)
            success = TRUE;
        else {
            memset(&reply, 0, sizeof(reply));
            reply.status.type   = RESMSG_STATUS;
            reply.status.id     = rset->id;
            reply.status.reqno  = resmsg->any.reqno;
            reply.status.errcod = errcod;
            reply.status.errmsg = errmsg;
            
            success = rp->any.error(rset, &reply, protodata);
        }
    }

    return success;
}


resproto_reply_t *resproto_reply_create(resmsg_type_t       type,
                                        uint32_t            serial,
                                        uint32_t            reqno,
                                        resset_t           *rset,
                                        resproto_status_t   status)
{
    resproto_any_t   *rp = &rset->resproto->any;
    resproto_reply_t *reply;
    resproto_reply_t *last;

    for (last = (resproto_reply_t*)&rp->replies; last->next; last = last->next)
        ;

    if ((reply = malloc(sizeof(resproto_reply_t))) != NULL) {
        memset(reply, 0, sizeof(resproto_reply_t));
        reply->type     = type;
        reply->serial   = serial;
        reply->reqno    = reqno;
        reply->callback = status;
        reply->rset     = rset;
                
        last->next = reply;
    }

    return reply;
}


void resproto_reply_destroy(void *ptr)
{
    resproto_reply_t *reply = (resproto_reply_t *)ptr;
    resset_t         *rset;
    resproto_t       *rp;
    resproto_reply_t *prev;

    if (ptr != NULL) {
        if ((rset  = reply->rset   ) != NULL &&
            (rp    = rset->resproto) != NULL    )
        {
            for (prev = (resproto_reply_t *)&rp->any.replies;
                 prev->next != NULL;
                 prev = prev->next)
            {
                if (prev->next == reply) {
                    prev->next  = reply->next;
                    reply->next = NULL;
                    break;
                }
            }
        }

        free(reply);
    }    
}

resproto_reply_t *resproto_reply_find(resproto_t *rp, uint32_t serial)
{
    resproto_reply_t *reply;

    for (reply = rp->any.replies;   reply != NULL;   reply = reply->next) {
        if (serial == reply->serial)
            break;
    }

    return reply;
}

static void manager_link_handler(resproto_t *rp, resproto_linkst_t state)
{
}

static void client_link_handler(resproto_t *rp, resproto_linkst_t state)
{
    resset_t          *rset, *next;
    resmsg_t           resmsg;
    resproto_handler_t handler;

    switch (state) {

    case RESPROTO_LINK_UP:
        if (rp->any.mgrup)
            rp->any.mgrup(rp);
        break;

    case RESPROTO_LINK_DOWN:
        handler = rp->any.handler[RESMSG_UNREGISTER];

        memset(&resmsg, 0, sizeof(resmsg));
        resmsg.possess.type = RESMSG_UNREGISTER;

        for (rset = rp->any.rsets;   rset != NULL;   rset = next) {
            next = rset->next;

            if (handler && rset->state == RESPROTO_RSET_STATE_CONNECTED) {
                resmsg.possess.id = rset->id;
                handler(&resmsg, rset, NULL);
            }

            rp->any.disconn(rset);
        }
        if (rp->any.rsets == NULL)
            printf("No hanging rset\n");
        break;

    default:
        break;
    }
}

static void message_receive(resmsg_t *resmsg,
                            resset_t *rset,
                            void     *protodata)
{
    resproto_t         *rp   = rset->resproto;
    resmsg_type_t       type = resmsg->type;
    resproto_handler_t  handler;

    if (type >= 0 && type < RESMSG_MAX && (handler = rp->any.handler[type])) {
        handler(resmsg, rset, protodata);
    }
}


static void resproto_list_add(resproto_t *rp)
{
    if (rp != NULL) {
        rp->any.next  = resproto_list;
        resproto_list = rp;
    }
}

static void resproto_list_delete(resproto_t *rp)
{
    resproto_t *prev;

    for (prev = (resproto_t *)&resproto_list;
         prev->any.next != NULL;
         prev = prev->any.next)
    {
        if (prev->any.next == rp) {
            prev->any.next = rp->any.next;
            free(rp);
        }
    }
}

resproto_t *resproto_list_iterate(resproto_t *rp)
{
    if (rp == NULL)
        rp = (resproto_t *)&resproto_list;

    return rp->any.next;
}



/* 
 * Local Variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim:set expandtab shiftwidth=4:
 */
