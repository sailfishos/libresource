#ifndef __RES_CONN_PRIVATE_H__
#define __RES_CONN_PRIVATE_H__

#include <res-conn.h>

resconn_t *resconn_init(resproto_role_t, resproto_transport_t, va_list);

resconn_reply_t *resconn_reply_create(resmsg_type_t, uint32_t, uint32_t,
                                      resset_t *, resproto_status_t);
void             resconn_reply_destroy(void *);
resconn_reply_t *resconn_reply_find(resconn_t *, uint32_t);


resconn_t *resconn_list_iterate(resconn_t *);

#endif /* __RES_CONN_PRIVATE_H__ */

/* 
 * Local Variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim:set expandtab shiftwidth=4:
 */
