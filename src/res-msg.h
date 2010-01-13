#ifndef __RES_MESSAGE_H__
#define __RES_MESSAGE_H__

#include <stdint.h>

#ifdef	__cplusplus
extern "C" {
#endif


#define RESMSG_BIT(n) (((uint32_t)1) << (n))

#define RESMSG_AUDIO_PLAYBACK      RESMSG_BIT(0)
#define RESMSG_VIDEO_PLAYBACK      RESMSG_BIT(1)
#define RESMSG_AUDIO_RECORDING     RESMSG_BIT(2)
#define RESMSG_VIDEO_RECORDING     RESMSG_BIT(3)

#define RESMSG_MODE_AUTO_RELEASE   RESMSG_BIT(0)


typedef enum resmsg_type_e {
    RESMSG_INVALID = -1,

    RESMSG_REGISTER,
    RESMSG_UNREGISTER,
    RESMSG_UPDATE,
    RESMSG_ACQUIRE,
    RESMSG_RELEASE,
    RESMSG_GRANT,
    RESMSG_ADVICE,
    RESMSG_AUDIO,

    RESMSG_MAX,

    RESMSG_STATUS = RESMSG_MAX,
    RESMSG_TRANSPORT_START,
} resmsg_type_t;

typedef struct {
    uint32_t          all;       /* all the resources */
    uint32_t          opt;       /* optional resources (subset of all) */
    uint32_t          share;     /* shareable resource value */
    uint32_t          mask;      /* shereable resource mask (subset of all) */
} resmsg_rset_t;

typedef enum {
    resmsg_method_equals = 0,
    resmsg_method_startswith,
    resmsg_method_matches
} resmsg_match_method_t;

typedef struct {
    resmsg_match_method_t  method;
    char                  *pattern;
} resmsg_match_t;

typedef struct {
    char             *name;
    resmsg_match_t    match;
} resmsg_property_t;

#define RESMSG_COMMON                                         \
    resmsg_type_t     type;      /* RESMSG_xxxx            */ \
    uint32_t          id;        /* resourse set ID        */ \
    uint32_t          reqno      /* request number, if any */

typedef struct {
    RESMSG_COMMON;
} resmsg_any_t;

typedef struct {
    RESMSG_COMMON;               /* RESMSG_[REGISTER|UPDATE] */
    resmsg_rset_t     rset;      /* resource set */
    char             *class;     /* optional application class */
    uint32_t          mode;      /* or'ed RESMSG_MODE_xxxx values */
} resmsg_record_t;

typedef struct {
    RESMSG_COMMON;               /* RESMG_[UNREGISTER|ACQUIRE|RELEASE] */
} resmsg_possess_t;

typedef struct {
    RESMSG_COMMON;               /* RESMG_[GRANT|ADVICE] */
    uint32_t          resrc;     /* effected resources */
} resmsg_notify_t;

typedef struct {
    RESMSG_COMMON;               /* RESMSG_AUDIO */
    uint32_t          pid;       /* PID of the streaming app, if any */
    resmsg_property_t property;  /* audio stream property */
} resmsg_audio_t;

typedef struct {
    RESMSG_COMMON;               /* RESMSG_STATUS */
    int32_t           errcod;    /* error code, if any */
    const char       *errmsg;    /* error message, if any */
} resmsg_status_t;


typedef union resmsg_u {
    resmsg_type_t     type;
    resmsg_any_t      any;
    resmsg_record_t   record;
    resmsg_possess_t  possess;
    resmsg_notify_t   notify;
    resmsg_audio_t    audio;
    resmsg_status_t   status;
} resmsg_t;


char *resmsg_dump_message(resmsg_t *, int, char *, int);
char *resmsg_type_str(resmsg_type_t);
char *resmsg_res_str(uint32_t, char *, int);
char *resmsg_mod_str(uint32_t, char *, int);
char *resmsg_match_method_str(resmsg_match_method_t);


#ifdef	__cplusplus
};
#endif

#endif /* __RES_MESSAGE_H__ */

/* 
 * Local Variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim:set expandtab shiftwidth=4:
 */
