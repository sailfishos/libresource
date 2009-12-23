#ifndef __RES_CONN_H__
#define __RES_CONN_H__

#include <dbus/dbus.h>

#include <res-proto.h>


typedef void        (*resconn_link_t)     (union resconn_u*,resproto_linkst_t);
typedef void        (*resconn_receive_t)  (resmsg_t *, resset_t *, void *);
typedef resset_t   *(*resconn_connect_t)  (union resconn_u *,resmsg_t *);
typedef void        (*resconn_disconn_t)  (resset_t *);
typedef int         (*resconn_send_t)     (resset_t *, resmsg_t *,
                                            resproto_status_t);
typedef int         (*resconn_error_t)    (resset_t *, resmsg_t *, void *);
typedef void        (*resconn_linkup_t)   (union resconn_u *);

typedef int         (*resconn_timercb_t)  (void *);
typedef void       *(*resconn_timer_add_t)(uint32_t,resconn_timercb_t,void*);
typedef void        (*resconn_timer_del_t)(void *);

typedef struct resconn_reply_s {
    struct resconn_reply_s  *next;
    uint32_t                 serial;    /* msg sequence number, if applies */
    resmsg_type_t            type;      /* msg type */
    uint32_t                 reqno;     /* request number, if applies */
    resproto_status_t        callback;
    resset_t                *rset;
    void                    *timer;     /* timer, if applies */
    void                    *data;      /* timer data, if applies */
} resconn_reply_t;             

#define RESCONN_QUEUE_LINK        \
    struct resconn_qitem_s *next; \
    struct resconn_qitem_s *prev

typedef struct resconn_qitem_s {
    RESCONN_QUEUE_LINK;
    char                    *peer;      /* sender name */
    void                    *data;      /* for the callback */
    resmsg_t                *msg;       /* message */
} resconn_qitem_t;

typedef struct {
    RESCONN_QUEUE_LINK;
} resconn_qhead_t;

#define RESCONN_COMMON                                 \
    union resconn_u         *next;                     \
    uint32_t                 id;                       \
    resproto_role_t          role;                     \
    resproto_transport_t     transp;                   \
    resset_t                *rsets;                    \
    resconn_reply_t         *replies;                  \
    resconn_link_t           link;                     \
    resconn_receive_t        receive;                  \
    resconn_connect_t        connect;                  \
    resconn_disconn_t        disconn;                  \
    resconn_send_t           send;                     \
    resconn_error_t          error;                    \
    int                     *valid;                    \
    resproto_handler_t       handler[RESMSG_MAX];      \
    resconn_linkup_t         mgrup;                    \
    int                      killed


typedef struct {
    RESCONN_COMMON;
} resconn_any_t;

typedef struct {
    RESCONN_COMMON;
    DBusConnection       *conn;
    char                 *dbusid;
    char                 *path;
} resconn_dbus_t;

typedef struct {
    RESCONN_COMMON;
    char                 *name;
    int                   busy;
    struct {
        resconn_qhead_t       head;
        void                 *timer;
    }                     queue;
    struct {
        resconn_timer_add_t   add;
        resconn_timer_del_t   del;
    }                     timer;
} resconn_internal_t;

typedef union resconn_u {
    resconn_any_t         any;
    resconn_dbus_t        dbus;
    resconn_internal_t    internal;
} resconn_t;


resconn_t *resconn_init(resproto_role_t, resproto_transport_t, ...);
resset_t  *resconn_connect(resconn_t *, resmsg_t *, resproto_status_t);
int        resconn_disconnect(resset_t *, resmsg_t *, resproto_status_t);

resconn_reply_t *resconn_reply_create(resmsg_type_t, uint32_t, uint32_t,
                                      resset_t *, resproto_status_t);
void             resconn_reply_destroy(void *);
resconn_reply_t *resconn_reply_find(resconn_t *, uint32_t);


resconn_t *resconn_list_iterate(resconn_t *);


#endif /* __RES_CONN_H__ */

/* 
 * Local Variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim:set expandtab shiftwidth=4:
 */
