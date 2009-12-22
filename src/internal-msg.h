#ifndef __RES_INTERNAL_MESSAGE_H__
#define __RES_INTERNAL_MESSAGE_H__

#include <stdint.h>

union resmsg_u;

union resmsg_u *resmsg_internal_copy_message(union resmsg_u *);
void            resmsg_internal_destroy_message(union resmsg_u *);


#endif /* __RES_INTERNAL_MESSAGE_H__ */

/* 
 * Local Variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim:set expandtab shiftwidth=4:
 */
