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


#ifndef __RES_SET_PRIVATE_H__
#define __RES_SET_PRIVATE_H__

#include <res-set.h>

resset_t *resset_create(union resconn_u*, const char*, uint32_t,
                        resset_state_t, const char *, uint32_t,
                        uint32_t, uint32_t, uint32_t, uint32_t);
void      resset_destroy(resset_t *);
void      resset_ref(resset_t *);
void      resset_unref(resset_t *);
void      resset_update_flags(resset_t *, uint32_t, uint32_t,
                              uint32_t,uint32_t);
resset_t *resset_find(union resconn_u *, const char *, uint32_t);

#endif /* __RES_SET_PRIVATE_H__ */

/* 
 * Local Variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim:set expandtab shiftwidth=4:
 */
