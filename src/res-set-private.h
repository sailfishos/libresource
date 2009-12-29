#ifndef __RES_SET_PRIVATE_H__
#define __RES_SET_PRIVATE_H__

#include <res-set.h>

resset_t *resset_create(union resconn_u*, const char*,
                        uint32_t, resset_state_t, const char *,
                        uint32_t, uint32_t, uint32_t);
void      resset_destroy(resset_t *);
void      resset_ref(resset_t *);
void      resset_unref(resset_t *);
resset_t *resset_find(union resconn_u *, const char *, uint32_t);

#endif /* __RES_SET_PRIVATE_H__ */

/* 
 * Local Variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim:set expandtab shiftwidth=4:
 */
