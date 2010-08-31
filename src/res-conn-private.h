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
