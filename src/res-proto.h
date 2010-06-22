/******************************************************************************/
/*  This file is part of libresource                                          */
/*                                                                            */
/*  Copyright (C) 2010 Nokia Corporation.                                     */
/*                                                                            */
/*  This library is free software; you can redistribute                       */
/*  it and/or modify it under the terms of the GNU Lesser General Public      */
/*  License as published by the Free Software Foundation                      */
/*  version 2.1 of the License.                                               */
/*                                                                            */
/*  This library is distributed in the hope that it will be useful,           */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU          */
/*  Lesser General Public License for more details.                           */
/*                                                                            */
/*  You should have received a copy of the GNU Lesser General Public          */
/*  License along with this library; if not, write to the Free Software       */
/*  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  */
/*  USA.                                                                      */
/******************************************************************************/

#ifndef __RES_PROTO_H__
#define __RES_PROTO_H__

#include <res-msg.h>
#include <res-set.h>

#ifdef	__cplusplus
extern "C" {
#endif

union resconn_u;

typedef enum {
    RESPROTO_ROLE_UNKNOWN = 0,
    RESPROTO_ROLE_MANAGER,
    RESPROTO_ROLE_CLIENT
} resproto_role_t;

typedef enum {
    RESPROTO_TRANSPORT_UNKNOWN = 0,
    RESPROTO_TRANSPORT_DBUS,
    RESPROTO_TRANSPORT_INTERNAL,
} resproto_transport_t;

typedef enum {
    RESPROTO_LINK_DOWN = 0,
    RESPROTO_LINK_UP   = 1
} resproto_linkst_t;


typedef void   (*resproto_handler_t) (resmsg_t *, resset_t *, void *);
typedef void   (*resproto_status_t)  (resset_t *, resmsg_t *);


union resconn_u * resproto_init(resproto_role_t, resproto_transport_t, ...);

int resproto_set_handler(union resconn_u *, resmsg_type_t, resproto_handler_t);

int resproto_send_message(resset_t *, resmsg_t *, resproto_status_t);
int resproto_reply_message(resset_t *,resmsg_t *,void *,int32_t,const char *);

#ifdef	__cplusplus
};
#endif


#endif /* __RES_PROTO_H__ */

/* 
 * Local Variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim:set expandtab shiftwidth=4:
 */
