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
