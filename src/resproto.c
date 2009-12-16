#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "resproto.h"
#include "dbus.h"


static resproto_t    *resproto_list;
static uint32_t       internal_reqno;

static void manager_link_handler(resproto_t *, resproto_linkst_t);
static void client_link_handler(resproto_t *, resproto_linkst_t);

static void manager_message_receive(resmsg_t *, resproto_rset_t *, void *);
static void client_message_receive(resmsg_t *, resproto_rset_t *, void *);

static int manager_init_internal(resproto_internal_t *, va_list);
static int client_init_internal(resproto_internal_t *, va_list);

static void        resproto_list_add(resproto_t *);
static void        resproto_list_delete(resproto_t *);
static resproto_t *resproto_list_find(uint32_t);

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
        rp->any.id     = ++id;
        rp->any.role   = role;
        rp->any.transp = transp;

        va_start(args, transp);
        
        switch (role) {

        case RESPROTO_ROLE_MANAGER:
            rp->any.link    = manager_link_handler;
            rp->any.receive = manager_message_receive;

            switch (transp) {
            case RESPROTO_TRANSPORT_DBUS:
                success = resproto_dbus_manager_init(&rp->dbus, args);
                break;
            case RESPROTO_TRANSPORT_INTERNAL:
                success = manager_init_internal(&rp->internal, args);
                break;
            default:
                success = FALSE;
            }

            break;
            
        case RESPROTO_ROLE_CLIENT:
            rp->any.link    = client_link_handler;
            rp->any.receive = client_message_receive;

            switch (transp) {
            case RESPROTO_TRANSPORT_DBUS:
                success = resproto_dbus_client_init(&rp->dbus, args);
                break;
            case RESPROTO_TRANSPORT_INTERNAL:
                success = client_init_internal(&rp->internal, args);
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
    if (type < 0 || type >= RESMSG_MAX || !handler)
        return FALSE;

   rp->any.handler[type] = handler;

    return TRUE;
}

resproto_rset_t *resproto_connect(resproto_t        *rp,
                                  resmsg_t          *resmsg,
                                  resproto_status_t  status)
{
    resproto_rset_t *rset;

    if (rp == NULL || rp->any.killed ||
        rp->any.role != RESPROTO_ROLE_CLIENT ||
        resmsg->type != RESMSG_REGISTER)
    {
        rset =  NULL;
    }
    else {
        switch (rp->any.transp) {
            
        case RESPROTO_ROLE_MANAGER:
            rset = rp->any.connect(rp, resmsg);
            rp->any.send(rset, resmsg, status);
            break;
            
        case RESPROTO_TRANSPORT_INTERNAL:
            rset = NULL;
            break;
            
        default:
            rset = NULL;
            break;

        }
    }

    return rset;
}

int resproto_disconnect(resproto_rset_t   *rset,
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

int resproto_send_message(resproto_rset_t   *rset,
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

int resproto_reply_message(resproto_rset_t *rset,
                           resmsg_t        *resmsg,
                           void            *data,
                           int32_t          errcod,
                           const char      *errmsg)
{
    resproto_t *rp = rset->resproto;
    resmsg_t    reply;
    int         success;

    if (!rset || !resmsg)
        success = FALSE;
    else {
        if (data == NULL)
            success = TRUE;
        else {
            memset(&reply, 0, sizeof(reply));
            reply.status.type   = RESMSG_STATUS;
            reply.status.id     = rset->id;
            reply.status.reqno  = resmsg->any.reqno;
            reply.status.errcod = errcod;
            reply.status.errmsg = errmsg;
            
            success = rp->any.error(rset, &reply, data);
        }
    }

    return success;
}


static void manager_link_handler(resproto_t *rp, resproto_linkst_t state)
{
}

static void client_link_handler(resproto_t *rp, resproto_linkst_t state)
{
    resproto_rset_t   *rset, *next;
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

static void manager_message_receive(resmsg_t        *resmsg,
                                    resproto_rset_t *rset,
                                    void            *data)
{
    resproto_t         *rp   = rset->resproto;
    resmsg_type_t       type = resmsg->type;
    resproto_handler_t  handler;

    if (type >= 0 && type < RESMSG_MAX && (handler = rp->any.handler[type])) {
        handler(resmsg, rset, data);
    }
}


static void client_message_receive(resmsg_t        *resmsg,
                                   resproto_rset_t *rset, 
                                   void            *data)
{
    resproto_t *rp = rset->resproto;

}


static int manager_init_internal(resproto_internal_t *rp, va_list args)
{
}

static int client_init_dbus(resproto_dbus_t *rp, va_list args)
{
    rp->conn = va_arg(args, DBusConnection *);
}

static int client_init_internal(resproto_internal_t *rp, va_list args)
{
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

static resproto_t *resproto_list_find(uint32_t id)
{
    resproto_t *rp;

    for (rp = resproto_list;  rp != NULL;   rp = rp->any.next) {
        if (rp->any.id == id)
            break;
    }


    return rp;
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
