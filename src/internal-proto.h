#ifndef __RES_INTERNAL_H__
#define __RES_INTERNAL_H__

#include <stdarg.h>
#include <res-conn.h>

#define RESPROTO_INTERNAL_MANAGER  "ResourceManager"

#define RESPROTO_INTERNAL_LINK     RESMSG_TRANSPORT_START

typedef struct {
    resmsg_type_t       type;      /* RESPROTO_INTERNAL_CONNECT */
    uint32_t            id;        /* resproto ID of the client */
    char               *name;      /* name of the client */
} resproto_internal_link_t;


int  resproto_internal_manager_init(resconn_internal_t *, va_list);
int  resproto_internal_client_init(resconn_internal_t *, va_list);



#endif /* __RES_INTERNAL_H__ */

/* 
 * Local Variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim:set expandtab shiftwidth=4:
 */
