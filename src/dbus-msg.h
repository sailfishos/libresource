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
