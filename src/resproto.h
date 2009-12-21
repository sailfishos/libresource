#ifndef __RES_PROTO_H__
#define __RES_PROTO_H__

#include <resmsg.h>
#include <resset.h>


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


typedef void        (*resproto_link_t) (union resproto_u *, resproto_linkst_t);
typedef void        (*resproto_linkup_t)   (union resproto_u *);
typedef void        (*resproto_receive_t)  (resmsg_t *, resset_t *, void *);
typedef void        (*resproto_status_t)   (resset_t *, resmsg_t *);
typedef resset_t   *(*resproto_connect_t)  (union resproto_u *,resmsg_t *);
typedef void        (*resproto_disconn_t)  (resset_t *);
typedef int         (*resproto_send_t)     (resset_t *, resmsg_t *,
                                            resproto_status_t);
typedef int         (*resproto_error_t)    (resset_t *, resmsg_t *, void *);
typedef void        (*resproto_handler_t)  (resmsg_t *, resset_t *, void *);

typedef int         (*resproto_timercb_t)  (void *);
typedef void       *(*resproto_timer_add_t)(uint32_t,resproto_timercb_t,void*);
typedef void        (*resproto_timer_del_t)(void *);

typedef struct resproto_reply_s {
    struct resproto_reply_s *next;
    uint32_t                 serial;    /* msg sequence number, if applies */
    resmsg_type_t            type;      /* msg type */
    uint32_t                 reqno;     /* request number, if applies */
    resproto_status_t        callback;
    resset_t                *rset;
    void                    *timer;     /* timer, if applies */
    void                    *data;      /* timer data, if applies */
} resproto_reply_t;             


#define RESPROTO_COMMON                                \
    union resproto_u        *next;                     \
    uint32_t                 id;                       \
    resproto_role_t          role;                     \
    resproto_transport_t     transp;                   \
    resset_t                *rsets;                    \
    resproto_reply_t        *replies;                  \
    resproto_link_t          link;                     \
    resproto_receive_t       receive;                  \
    resproto_connect_t       connect;                  \
    resproto_disconn_t       disconn;                  \
    resproto_send_t          send;                     \
    resproto_error_t         error;                    \
    int                     *valid;                    \
    resproto_handler_t       handler[RESMSG_MAX];      \
    resproto_linkup_t        mgrup;                    \
    int                      killed


typedef struct {
    RESPROTO_COMMON;
} resproto_any_t;

typedef struct {
    RESPROTO_COMMON;
    DBusConnection       *conn;
    char                 *dbusid;
    char                 *path;
} resproto_dbus_t;

typedef struct {
    RESPROTO_COMMON;
    char                 *name;
    struct {
        resproto_timer_add_t  add;
        resproto_timer_del_t  del;
    }                     timer;
} resproto_internal_t;

typedef union resproto_u {
    resproto_any_t        any;
    resproto_dbus_t       dbus;
    resproto_internal_t   internal;
} resproto_t;



resproto_t *resproto_init(resproto_role_t, resproto_transport_t, ...);
int resproto_set_handler(resproto_t *, resmsg_type_t , resproto_handler_t);

resset_t *resproto_connect(resproto_t *, resmsg_t *, resproto_status_t);
int resproto_disconnect(resset_t *, resmsg_t *, resproto_status_t);
int resproto_send_message(resset_t *, resmsg_t *, resproto_status_t);
int resproto_reply_message(resset_t *,resmsg_t *,void *,int32_t,const char *);

resproto_reply_t *resproto_reply_create(resmsg_type_t, uint32_t, uint32_t,
                                        resset_t *, resproto_status_t);
void resproto_reply_destroy(void *);
resproto_reply_t *resproto_reply_find(resproto_t *, uint32_t);


resproto_t *resproto_list_iterate(resproto_t *);

#endif /* __RES_PROTO_H__ */

/* 
 * Local Variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim:set expandtab shiftwidth=4:
 */
