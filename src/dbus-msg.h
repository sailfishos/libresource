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

#ifndef __RES_DBUS_MESSAGE_H__
#define __RES_DBUS_MESSAGE_H__

#include <stdint.h>
#include <dbus/dbus.h>

union resmsg_u;

DBusMessage    *resmsg_dbus_compose_message(const char *, const char *,
					    const char *, const char *,
					    union resmsg_u *);
DBusMessage    *resmsg_dbus_reply_message(DBusMessage *, union resmsg_u *);
union resmsg_u *resmsg_dbus_parse_message(DBusMessage *, union resmsg_u *);

#endif /* __RES_DBUS_MESSAGE_H__ */

/* 
 * Local Variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim:set expandtab shiftwidth=4:
 */
