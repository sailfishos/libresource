#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <res-conn.h>

static resconn_t     *resconn_list;

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


static void manager_link_handler(resconn_t *, resproto_linkst_t);
static void client_link_handler(resconn_t *, resproto_linkst_t);

static void message_receive(resmsg_t *, resset_t *, void *);

static void resconn_list_add(resconn_t *);
static void resconn_list_delete(resconn_t *);



resconn_t *resconn_init(resproto_role_t       role,
                        resproto_transport_t  transp,
                        ...  /* role & transport specific args */ )
{
    static uint32_t  id;

    va_list    args;
    resconn_t *rp;
    int        success;

    if ((rp = malloc(sizeof(resconn_t))) != NULL) {

        memset(rp, 0, sizeof(resconn_t));
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

        resconn_list_add(rp);
    }

    return rp;

 failed:
    free(rp);
    return NULL;
}


resset_t *resconn_connect(resconn_t         *rp,
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

int resconn_disconnect(resset_t          *rset,
                       resmsg_t          *resmsg,
                       resproto_status_t  status)
{
    resconn_t  *rp = rset->resconn;
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


resconn_reply_t *resconn_reply_create(resmsg_type_t       type,
                                      uint32_t            serial,
                                      uint32_t            reqno,
                                      resset_t           *rset,
                                      resproto_status_t   status)
{
    resconn_any_t   *rp = &rset->resconn->any;
    resconn_reply_t *reply;
    resconn_reply_t *last;

    for (last = (resconn_reply_t *)&rp->replies; last->next; last = last->next)
        ;

    if ((reply = malloc(sizeof(resconn_reply_t))) != NULL) {
        memset(reply, 0, sizeof(resconn_reply_t));
        reply->type     = type;
        reply->serial   = serial;
        reply->reqno    = reqno;
        reply->callback = status;
        reply->rset     = rset;
                
        last->next = reply;
    }

    return reply;
}


void resconn_reply_destroy(void *ptr)
{
    resconn_reply_t *reply = (resconn_reply_t *)ptr;
    resset_t        *rset;
    resconn_t       *rp;
    resconn_reply_t *prev;

    if (ptr != NULL) {
        if ((rset  = reply->rset  ) != NULL &&
            (rp    = rset->resconn) != NULL    )
        {
            for (prev = (resconn_reply_t *)&rp->any.replies;
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

resconn_reply_t *resconn_reply_find(resconn_t *rp, uint32_t serial)
{
    resconn_reply_t *reply;

    for (reply = rp->any.replies;   reply != NULL;   reply = reply->next) {
        if (serial == reply->serial)
            break;
    }

    return reply;
}


static void manager_link_handler(resconn_t *rp, resproto_linkst_t state)
{
}

static void client_link_handler(resconn_t *rp, resproto_linkst_t state)
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
    resconn_t          *rp   = rset->resconn;
    resmsg_type_t       type = resmsg->type;
    resproto_handler_t  handler;

    if (type >= 0 && type < RESMSG_MAX && (handler = rp->any.handler[type])) {
        handler(resmsg, rset, protodata);
    }
}


static void resconn_list_add(resconn_t *rp)
{
    if (rp != NULL) {
        rp->any.next  = resconn_list;
        resconn_list = rp;
    }
}

static void resconn_list_delete(resconn_t *rp)
{
    resconn_t *prev;

    for (prev = (resconn_t *)&resconn_list;
         prev->any.next != NULL;
         prev = prev->any.next)
    {
        if (prev->any.next == rp) {
            prev->any.next = rp->any.next;
            free(rp);
        }
    }
}

resconn_t *resconn_list_iterate(resconn_t *rp)
{
    if (rp == NULL)
        rp = (resconn_t *)&resconn_list;

    return rp->any.next;
}

/* 
 * Local Variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim:set expandtab shiftwidth=4:
 */
