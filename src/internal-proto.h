/*************************************************************************
This file is part of libresource

Copyright (C) 2010 Nokia Corporation.

This library is free software; you can redistribute
it and/or modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation
version 2.1 of the License.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
USA.
*************************************************************************/


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
