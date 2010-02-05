#ifndef __LIB_RESOURCE_GLUE_H__
#define __LIB_RESOURCE_GLUE_H__

#include <stdint.h>

#include <res-conn.h>

DBusConnection *resource_get_dbus_bus(DBusBusType, DBusError *);
void           *resource_timer_add(uint32_t, resconn_timercb_t,void *);
void            resource_timer_del(void *);

#endif /* __LIB_RESOURCE_GLUE_H__ */

/* 
 * Local Variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim:set expandtab shiftwidth=4:
 */
