#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <res-proto.h>
#include "res-conn-private.h"
#include "dbus-msg.h"
#include "dbus-proto.h"
#include "internal-msg.h"
#include "internal-proto.h"
#include "visibility.h"


static void message_receive(resmsg_t *, resset_t *, void *);

EXPORT resconn_t *resproto_init(resproto_role_t       role,
                                resproto_transport_t  transp,
                                ...    /* role & transport specific args */ )
{
    va_list    args;
    resconn_t *rcon;

    va_start(args, transp);

    rcon = resconn_init(role, transp, args);

    if (rcon != NULL) {
        rcon->any.receive = message_receive;        
    }

    va_end(args);

    return rcon;
}


EXPORT int resproto_set_handler(resconn_t           *rcon,
                                resmsg_type_t        type,
                                resproto_handler_t   handler)
{
    if (type < 0 || type >= RESMSG_MAX || !rcon->any.valid[type] || !handler)
        return FALSE;

   rcon->any.handler[type] = handler;

    return TRUE;
}

EXPORT int resproto_send_message(resset_t          *rset,
                                 resmsg_t          *resmsg,
                                 resproto_status_t  status)
{
    resconn_t       *rcon = rset->resconn;
    resmsg_type_t    type = resmsg->type;
    int              success;

    if (rset->state != RESPROTO_RSET_STATE_CONNECTED ||
        type == RESMSG_REGISTER || type == RESMSG_UNREGISTER)
        success = FALSE;
    else {
        resmsg->any.id = rset->id;
        success = rcon->any.send(rset, resmsg, status);
    }

    return success;
}

EXPORT int resproto_reply_message(resset_t   *rset,
                                  resmsg_t   *resmsg,
                                  void       *protodata,
                                  int32_t     errcod,
                                  const char *errmsg)
{
    resconn_t *rcon = rset->resconn;
    resmsg_t   reply;
    int        success;

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
            
            success = rcon->any.error(rset, &reply, protodata);
        }
    }

    return success;
}


static void message_receive(resmsg_t *resmsg,
                            resset_t *rset,
                            void     *protodata)
{
    resconn_t          *rcon = rset->resconn;
    resmsg_type_t       type = resmsg->type;
    resproto_handler_t  handler;

    if (type >= 0 && type < RESMSG_MAX && (handler = rcon->any.handler[type])){
        handler(resmsg, rset, protodata);
    }
}



/* 
 * Local Variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim:set expandtab shiftwidth=4:
 */
