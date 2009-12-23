#ifndef __RES_DBUS_PROTO_H__
#define __RES_DBUS_PROTO_H__

#include <stdarg.h>
#include <res-conn.h>

/* D-Bus names */
#define RESPROTO_DBUS_ADMIN_NAME                "org.freedesktop.DBus"
#define RESPROTO_DBUS_MANAGER_NAME              "org.maemo.resource.manager"

/* D-Bus pathes */
#define RESPROTO_DBUS_ADMIN_PATH                 "/org/freedesktop/DBus"
#define RESPROTO_DBUS_MANAGER_PATH               "/org/maemo/resource/manager"
#define RESPROTO_DBUS_CLIENT_PATH                "/org/maemo/resource/client%d"

/* D-Bus interfaces */
#define RESPROTO_DBUS_ADMIN_INTERFACE            "org.freedesktop.DBus"
#define RESPROTO_DBUS_MANAGER_INTERFACE          "org.maemo.resource.manager"
#define RESPROTO_DBUS_CLIENT_INTERFACE           "org.maemo.resource.client"

/* D-Bus methods */
#define RESPROTO_DBUS_NAME_OWNER_CHANGED_SIGNAL  "NameOwnerChanged"
#define RESPROTO_DBUS_REGISTER_METHOD            "register"
#define RESPROTO_DBUS_UREGISTER_METHOD           "unregister"
#define RESPROTO_DBUS_UPDATE_METHOD              "update"
#define RESPROTO_DBUS_ACQUIRE_METHOD             "acquire"
#define RESPROTO_DBUS_RELEASE_METHOD             "release"
#define RESPROTO_DBUS_GRANT_METHOD               "grant"
#define RESPROTO_DBUS_ADVICE_METHOD              "advice"

int resproto_dbus_manager_init(resconn_dbus_t *, va_list);
int resproto_dbus_client_init(resconn_dbus_t *, va_list);


#endif /* __RES_DBUS_PROTO_H__ */

/* 
 * Local Variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim:set expandtab shiftwidth=4:
 */
