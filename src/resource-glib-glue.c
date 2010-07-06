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


#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "resource-glue.h"
#include "visibility.h"

DBusConnection *resource_get_dbus_bus(DBusBusType type, DBusError *err)
{
    DBusConnection *conn = NULL;

    if ((conn = dbus_bus_get(type, err)) != NULL) {
        dbus_connection_setup_with_g_main(conn, NULL);
    }

    return conn;
}

void *resource_timer_add(uint32_t delay, resconn_timercb_t cbfunc,void *cbdata)
{
    return NULL;
}

void resource_timer_del(void *timer)
{
}

/* 
 * Local Variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim:set expandtab shiftwidth=4:
 */
