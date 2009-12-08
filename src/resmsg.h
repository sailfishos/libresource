#ifndef __RES_MESSAGE_H__
#define __RES_MESSAGE_H__

#include <stdint.h>
#include <dbus/dbus.h>

#define RESMSG_BIT(n) (((uint32_t)1) << (n))

#define RESMSG_AUDIO_PLAYBACK      RESMSG_BIT(0)
#define RESMSG_VIDEO_PLAYBACK      RESMSG_BIT(1)
#define RESMSG_AUDIO_RECORDING     RESMSG_BIT(2)
#define RESMSG_VIDEO_RECORDING     RESMSG_BIT(3)


typedef enum {
    RESMSG_INVALID = 0,
    RESMSG_REGISTER,
    RESMSG_UNREGISTER,
    RESMSG_UPDATE,
    RESMSG_ACQUIRE,
    RESMSG_RELEASE,
    RESMSG_GRANT,
    RESMSG_ADVICE,
} resmsg_type_t;

typedef struct {
    uint32_t          all;       /* all the resources */
    uint32_t          share;     /* shareable resources (subset of all) */
    uint32_t          opt;       /* optional resources (subset of all) */
} resmsg_rset_t;

typedef struct {
    resmsg_type_t     type;      /* RESMSG_[REGISTER|UNREGISTER|UPDATE] */
    uint32_t          id;        /* ID of the resource set */
    resmsg_rset_t     rset;      /* resource set */
    char             *class;     /* optional application class */
} resmsg_record_t;

typedef struct {
    resmsg_type_t     type;      /* RESMG_[ACQUIRE|RELEASE] */
    uint32_t          id;        /* ID of the resource set */
    uint32_t          reqno;     /* request number */
} resmsg_possess_t;

typedef struct {
    resmsg_type_t     type;      /* RESMG_[GRANT|ADVICE] */
    uint32_t          id;        /* ID of the resource set */
    uint32_t          resrc;     /* effected resources */
    uint32_t          reqno;     /* corresponding request number, if any */
} resmsg_notify_t;

typedef union {
    resmsg_type_t     type;
    resmsg_record_t   record;
    resmsg_possess_t  possess;
    resmsg_notify_t   notify;
} resmsg_t;


DBusMessage *resmsg_compose_dbus_message(const char *, const char *,
                                         const char *, const char *,
                                         resmsg_t *);
resmsg_t    *resmsg_parse_dbus_message(DBusMessage *, resmsg_t *);

#endif /* __RES_MESSAGE_H__ */

/* 
 * Local Variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim:set expandtab shiftwidth=4:
 */
