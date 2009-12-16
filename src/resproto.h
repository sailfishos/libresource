#ifndef __RES_PROTO_H__
#define __RES_PROTO_H__

#include <resmsg.h>


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

typedef enum {
    RESPROTO_RSET_STATE_CREATED = 0,
    RESPROTO_RSET_STATE_CONNECTED,
    RESPROTO_RSET_STATE_KILLED,
} resproto_rset_state_t;

typedef struct resproto_rset_s {
    struct resproto_rset_s  *next;
    int32_t                  refcnt;
    union resproto_u        *resproto;
    char                    *peer;
    uint32_t                 id;
    resproto_rset_state_t    state;
    void                    *userdata;
} resproto_rset_t;

typedef void             (*resproto_link_t)    (union resproto_u *,
                                                resproto_linkst_t);
typedef void             (*resproto_linkup_t)  (union resproto_u *);
typedef void             (*resproto_receive_t) (resmsg_t *, resproto_rset_t *,
                                                void *);
typedef void             (*resproto_status_t)  (resproto_rset_t *, resmsg_t *);
typedef resproto_rset_t *(*resproto_connect_t) (union resproto_u *,resmsg_t *);
typedef void             (*resproto_disconn_t) (resproto_rset_t *);
typedef int              (*resproto_send_t)    (resproto_rset_t *, resmsg_t *,
                                                resproto_status_t);
typedef int              (*resproto_error_t)   (resproto_rset_t *, resmsg_t *,
                                                void *);
typedef void             (*resproto_handler_t) (resmsg_t *, resproto_rset_t *,
                                                void *);


typedef struct resproto_reply_s {
    struct resproto_reply_s *next;
    uint32_t                 serial;    /* msg sequence number, if applies */
    resmsg_type_t            type;      /* msg type */
    uint32_t                 reqno;     /* request number, if applies */
    resproto_status_t        callback;
    resproto_rset_t         *rset; 
} resproto_reply_t;             


#define RESPROTO_COMMON                                \
    union resproto_u        *next;                     \
    uint32_t                 id;                       \
    resproto_role_t          role;                     \
    resproto_transport_t     transp;                   \
    resproto_rset_t         *rsets;                    \
    resproto_reply_t        *replies;                  \
    resproto_link_t          link;                     \
    resproto_receive_t       receive;                  \
    resproto_connect_t       connect;                  \
    resproto_disconn_t       disconn;                  \
    resproto_send_t          send;                     \
    resproto_error_t         error;                    \
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
} resproto_internal_t;

typedef union resproto_u {
    resproto_any_t        any;
    resproto_dbus_t       dbus;
    resproto_internal_t   internal;
} resproto_t;



resproto_t *resproto_init(resproto_role_t, resproto_transport_t, ...);
int resproto_set_handler(resproto_t *, resmsg_type_t , resproto_handler_t);

resproto_rset_t *resproto_connect(resproto_t *, resmsg_t *, resproto_status_t);
int resproto_disconnect(resproto_rset_t *, resmsg_t *, resproto_status_t);
int resproto_send_message(resproto_rset_t *, resmsg_t *, resproto_status_t);
int resproto_reply_message(resproto_rset_t *, resmsg_t *, void *,
                           int32_t, const char *);


resproto_t *resproto_list_iterate(resproto_t *);

#endif /* __RES_PROTO_H__ */

/* 
 * Local Variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim:set expandtab shiftwidth=4:
 */
