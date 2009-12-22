#ifndef __RES_SET_H__
#define __RES_SET_H__

#include <stdint.h>

union resproto_u;

typedef enum {
    RESPROTO_RSET_STATE_CREATED = 0,
    RESPROTO_RSET_STATE_CONNECTED,
    RESPROTO_RSET_STATE_KILLED,
} resset_state_t;

typedef struct resset_s {
    struct resset_s  *next;
    int32_t           refcnt;
    union resproto_u *resproto;
    char             *peer;
    uint32_t          id;
    resset_state_t    state;
    struct {
        uint32_t all;
        uint32_t share;
        uint32_t opt;
    }                 flags;
    void             *userdata;
} resset_t;

resset_t *resset_create(union resproto_u*, const char*,
                        uint32_t, resset_state_t,
                        uint32_t, uint32_t, uint32_t);
void      resset_destroy(resset_t *);
void      resset_ref(resset_t *);
void      resset_unref(resset_t *);
resset_t *resset_find(union resproto_u *, const char *, uint32_t);


#endif /* __RES_SET_H__ */

/* 
 * Local Variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim:set expandtab shiftwidth=4:
 */
