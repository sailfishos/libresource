#ifndef __RES_PROTO_H__
#define __RES_PROTO_H__

#include <res-msg.h>
#include <res-set.h>

union resconn_u;

typedef enum {
    RESPROTO_ROLE_UNKNOWN = 0,
    RESPROTO_ROLE_MANAGER,
    RESPROTO_ROLE_CLIENT
} resproto_role_t;

typedef enum {
    RESPROTO_TRANSPORT_UNKNOWN = 0,
    RESPROTO_TRANSPORT_DBUS,
    RESPROTO_TRANSPORT_INTERNAL,
} resproto_transport_t;

typedef enum {
    RESPROTO_LINK_DOWN = 0,
    RESPROTO_LINK_UP   = 1
} resproto_linkst_t;


typedef void   (*resproto_handler_t) (resmsg_t *, resset_t *, void *);
typedef void   (*resproto_status_t)  (resset_t *, resmsg_t *);


union resconn_u * resproto_init(resproto_role_t, resproto_transport_t, ...);

int resproto_set_handler(union resconn_u *, resmsg_type_t, resproto_handler_t);

int resproto_send_message(resset_t *, resmsg_t *, resproto_status_t);
int resproto_reply_message(resset_t *,resmsg_t *,void *,int32_t,const char *);


#endif /* __RES_PROTO_H__ */

/* 
 * Local Variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim:set expandtab shiftwidth=4:
 */
