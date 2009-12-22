#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "dbus.h"


/* 
 * local function prototypes
 */
static resset_t *connect_to_manager(resproto_t *, resmsg_t*);
static resset_t *connect_fail(resproto_t *, resmsg_t *);
static int       send_message(resset_t *, resmsg_t *, resproto_status_t);
static int       send_error(resset_t *, resmsg_t *, void *);
static void      status_method(DBusPendingCall *, void *);

static resproto_t *find_resproto(DBusConnection *);

static int watch_manager(resproto_dbus_t *, int);
static int watch_client(resproto_dbus_t *, const char *, int);
static int remove_filter(resproto_dbus_t *, char *);
static int add_filter(resproto_dbus_t *, char *);
static int request_name(resproto_dbus_t *, char *);
static int register_manager_object(resproto_dbus_t *);

static DBusHandlerResult client_name_changed(DBusConnection *,
                                             DBusMessage *, void *);
static DBusHandlerResult manager_name_changed(DBusConnection *,
                                              DBusMessage *, void *);
static DBusHandlerResult manager_method(DBusConnection *,DBusMessage *,void *);
static char *method_name(resmsg_type_t);

/* 
 * local storage
 */
static int          timeout = -1;     /* message timeout in msec's */

int resproto_dbus_manager_init(resproto_dbus_t *rp, va_list args)
{
    DBusConnection    *conn  = va_arg(args, DBusConnection *);
    const char        *name  = dbus_bus_get_unique_name(conn);

    int success = FALSE;

    rp->conn  = conn;

    if (dbus_connection_add_filter(conn, manager_name_changed,NULL, NULL) &&
        request_name(rp, RESPROTO_DBUS_MANAGER_NAME)                      &&
        register_manager_object(rp)                                          )
    {
        rp->connect = connect_fail;
        rp->disconn = resset_destroy;
        rp->send    = send_message;
        rp->error   = send_error;
        rp->dbusid  = strdup(name);
        rp->path    = strdup(RESPROTO_DBUS_MANAGER_PATH);

        success = TRUE;
    }

    return success;
}



int resproto_dbus_client_init(resproto_dbus_t *rp, va_list args)
{
    static int      client_no;

    resproto_linkup_t  mgrup = va_arg(args, resproto_linkup_t);
    DBusConnection    *conn  = va_arg(args, DBusConnection *);
    const char        *name  = dbus_bus_get_unique_name(conn);
    int                success = FALSE;
    char               path[1024];

    rp->conn  = conn;
    rp->mgrup = mgrup;

    if (dbus_connection_add_filter(conn, client_name_changed,NULL, NULL) &&
        watch_manager(rp, TRUE)                                             )
    {    
        snprintf(path, sizeof(path), RESPROTO_DBUS_CLIENT_PATH, client_no++);
        
        rp->connect = connect_to_manager;
        rp->disconn = resset_destroy;
        rp->send    = send_message;
        rp->error   = send_error;
        rp->dbusid  = strdup(name);
        rp->path    = strdup(path);

        success = TRUE;
    }

    return success;
}

static resset_t *connect_to_manager(resproto_t *rp, resmsg_t *resmsg)
{
    char          *name   = RESPROTO_DBUS_MANAGER_NAME;
    resmsg_rset_t *flags = &resmsg->record.rset;
    uint32_t       id     = resmsg->any.id;
    resset_t      *rset;

    if ((rset = resset_find(rp, name, id)) == NULL)
        rset = resset_create(rp, name, id, RESPROTO_RSET_STATE_CREATED,
                             flags->all, flags->share, flags->opt);

    return rset;
}

static resset_t *connect_fail(resproto_t *rp, resmsg_t *resmsg)
{
    (void)rp;
    (void)resmsg;

    return NULL;
}

static int send_message(resset_t          *rset,
                        resmsg_t          *resmsg,
                        resproto_status_t  status)
{
    resproto_dbus_t   *rp;
    DBusMessage       *dbusmsg;
    char              *dest;
    char              *path;
    char              *iface;
    char              *method;
    char               buf[1024];
    resmsg_type_t      type;
    uint32_t           serial;
    uint32_t           reqno;
    DBusPendingCall   *pend;
    int                need_reply;
    resproto_reply_t  *reply;
    int                success;

    if (!rset || !resmsg)
        return FALSE;

    rp = &rset->resproto->dbus;
    
    switch (rp->role) {
        
    case RESPROTO_ROLE_MANAGER:
        snprintf(buf, sizeof(buf), RESPROTO_DBUS_CLIENT_PATH, resmsg->any.id);
        path  = buf;
        iface = RESPROTO_DBUS_CLIENT_INTERFACE;
        break;
        
    case RESPROTO_ROLE_CLIENT:
        path  = RESPROTO_DBUS_MANAGER_PATH;
        iface = RESPROTO_DBUS_MANAGER_INTERFACE;
        break;
        
    default:
        return FALSE;
    }


    if (!(dest   = rset->peer)  ||
        !(method = method_name(resmsg->type)) ||
        !(dbusmsg= resmsg_compose_dbus_message(dest,path,iface,method,resmsg)))
    {
        success = FALSE;
    }
    else {
        if (rp->role != RESPROTO_ROLE_CLIENT)
            need_reply = status ? TRUE : FALSE;
        else {
            switch (resmsg->any.type) {
            case RESMSG_REGISTER:    need_reply = TRUE;                  break;
            case RESMSG_UNREGISTER:  need_reply = TRUE;                  break;
            default:                 need_reply = status ? TRUE : FALSE; break;
            }
        }

        if (!need_reply)
            success = dbus_connection_send(rp->conn, dbusmsg, NULL);
        else {
            do {
                success = dbus_connection_send_with_reply(rp->conn, dbusmsg,
                                                          &pend, timeout);
                if (!success)
                    break;

                type   = resmsg->type;
                serial = dbus_message_get_serial(dbusmsg);
                reqno  = resmsg->any.reqno;
                reply  = resproto_reply_create(type,serial,reqno,rset,status);

                success = dbus_pending_call_set_notify(pend, status_method,
                                                       reply,
                                                       resproto_reply_destroy);
            } while(0);
        }

        if (success)
            resset_ref(rset);

        dbus_message_unref(dbusmsg);
    }
    
    return success;
}

static int send_error(resset_t *rset, resmsg_t *resreply, void *data)
{
    resproto_t     *rp        = rset->resproto;
    DBusConnection *conn      = rp->dbus.conn; 
    DBusMessage    *dbusmsg   = (DBusMessage *)data;
    dbus_uint32_t   serial    = dbus_message_get_serial(dbusmsg);
    DBusMessage    *dbusreply = resmsg_reply_dbus_message(dbusmsg, resreply);
    int             success;

    dbus_connection_send(conn, dbusreply, &serial);
    dbus_message_unref(dbusreply);
    dbus_message_unref(dbusmsg);

    return TRUE;
}

static void status_method(DBusPendingCall *pend, void *data)
{
    resproto_reply_t *reply   = (resproto_reply_t *)data;
    DBusMessage      *dbusmsg = dbus_pending_call_steal_reply(pend);
    resset_t         *rset;
    resproto_t       *rp;
    resmsg_t          resmsg;
    const char       *errmsg;
    int               success;

    if (reply && dbusmsg){
        rset = reply->rset;
        rp   = rset->resproto;
        
        if (dbus_message_get_type(dbusmsg) == DBUS_MESSAGE_TYPE_ERROR) {
            errmsg = dbus_message_get_error_name(dbusmsg);

            if (!strncmp(errmsg, "org.freedesktop.", 16))
                errmsg += 16;
            else if (!strncmp(errmsg, "com.nokia.", 10))
                errmsg += 10;

            memset(&resmsg, 0, sizeof(resmsg));
            resmsg.status.type   = RESMSG_STATUS;
            resmsg.status.id     = rset->id;
            resmsg.status.reqno  = reply->reqno;
            resmsg.status.errcod = -1;
            resmsg.status.errmsg = errmsg ? errmsg : "<unidentified error>";
        }
        else {
            if (!resmsg_parse_dbus_message(dbusmsg, &resmsg)            ||
                reply->serial != dbus_message_get_reply_serial(dbusmsg) ||
                resmsg.status.type  != RESMSG_STATUS                    ||
                resmsg.status.id    != rset->id                         ||
                resmsg.status.reqno != reply->reqno                       )
            {
                memset(&resmsg, 0, sizeof(resmsg));
                resmsg.status.type   = RESMSG_STATUS;
                resmsg.status.id     = rset->id;
                resmsg.status.reqno  = reply->reqno;
                resmsg.status.errcod = -1;
                resmsg.status.errmsg = "<peer error>";
            }
        }

        if (rp->any.role == RESPROTO_ROLE_CLIENT) {
            switch (reply->type) {

            case RESMSG_REGISTER:
                if (!resmsg.status.errcod)
                    rset->state = RESPROTO_RSET_STATE_CONNECTED;
                else
                    rset->state = RESPROTO_RSET_STATE_KILLED;
                break;

            case RESMSG_UNREGISTER:
                if (resmsg.status.errcod) {
                    resset_ref(rset);
                    rset->state = RESPROTO_RSET_STATE_CONNECTED;
                }
                break;

            default:
                break;
            }
        }
        
        if (reply->callback != NULL)
            reply->callback(rset, &resmsg);

        resset_unref(rset);
    }

    if (dbusmsg)
        dbus_message_unref(dbusmsg);

    dbus_pending_call_unref(pend);
}


static resproto_t *find_resproto(DBusConnection *conn)
{
    resproto_t *rp = NULL;

    while ((rp = resproto_list_iterate(rp)) != NULL) {
        if (rp->any.transp == RESPROTO_TRANSPORT_DBUS && rp->dbus.conn == conn)
            break;
    }
    
    return rp;
}

static int watch_manager(resproto_dbus_t *rp, int watchit)
{
    static char *filter =
        "type='signal',"
        "sender='"    RESPROTO_DBUS_ADMIN_NAME                "',"
        "interface='" RESPROTO_DBUS_ADMIN_INTERFACE           "',"
        "member='"    RESPROTO_DBUS_NAME_OWNER_CHANGED_SIGNAL "',"
        "path='"      RESPROTO_DBUS_ADMIN_PATH                "',"
        "arg0='"      RESPROTO_DBUS_MANAGER_NAME              "'";

    int success;

    if (watchit)
        success = add_filter(rp, filter);
    else
        success = remove_filter(rp, filter);
    
    return success;
}


static int watch_client(resproto_dbus_t *rp, const char *dbusid, int watchit)
{
    static char *filter_fmt =
        "type='signal',"
        "sender='"    RESPROTO_DBUS_ADMIN_NAME                "',"
        "interface='" RESPROTO_DBUS_ADMIN_INTERFACE           "',"
        "member='"    RESPROTO_DBUS_NAME_OWNER_CHANGED_SIGNAL "',"
        "path='"      RESPROTO_DBUS_ADMIN_PATH                "',"
        "arg0='%s',arg1='%s',arg2=''";

    char filter[1024];
    int  success;

    snprintf(filter, sizeof(filter), filter_fmt, dbusid, dbusid);
    
    if (watchit)
        success = add_filter(rp, filter);
    else
        success = remove_filter(rp, filter);
    
    return success;
}


static int add_filter(resproto_dbus_t *rp, char *filter)
{
    DBusError  err;

    dbus_error_init(&err);
    dbus_bus_add_match(rp->conn, filter, &err);

    if (dbus_error_is_set(&err)) {
        dbus_error_free(&err);
        return FALSE;
    }

    return TRUE;
}

static int remove_filter(resproto_dbus_t *rp, char *filter)
{
    dbus_bus_remove_match(rp->conn, filter, NULL);

    return TRUE;
}

static int request_name(resproto_dbus_t *rp, char *name)
{
    DBusError  err;
    int        retval;
    int        success;

    dbus_error_init(&err);

    retval = dbus_bus_request_name(rp->conn, name,
                                   DBUS_NAME_FLAG_REPLACE_EXISTING, &err);

    if (retval == DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
        success = TRUE;
    else {
        success = FALSE;

        if (dbus_error_is_set(&err)) {
            dbus_error_free(&err);
        }
    }

    return success;
}

static int register_manager_object(resproto_dbus_t *rp)
{
    static struct DBusObjectPathVTable method = {
        .message_function = manager_method
    };

    int success;

    success = dbus_connection_register_object_path(rp->conn,
                                                   RESPROTO_DBUS_MANAGER_PATH,
                                                   &method, NULL);
    
    return success;
}


static DBusHandlerResult manager_name_changed(DBusConnection *conn,
                                              DBusMessage    *msg,
                                              void           *user_data)
{
    (void)conn;
    (void)user_data;

    char              *sender;
    char              *before;
    char              *after;
    resproto_t        *rp;
    int                success;

    success = dbus_message_is_signal(msg, RESPROTO_DBUS_ADMIN_INTERFACE,
                                     RESPROTO_DBUS_NAME_OWNER_CHANGED_SIGNAL);

    if (!success)
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;


    success = dbus_message_get_args(msg, NULL,
                                    DBUS_TYPE_STRING, &sender,
                                    DBUS_TYPE_STRING, &before,
                                    DBUS_TYPE_STRING, &after,
                                    DBUS_TYPE_INVALID);
    
    if (success && sender != NULL && before != NULL) {
        if ((rp = find_resproto(conn)) != NULL) {
            if (!after || !strcmp(after, "")) {
                /* client is gone */
                if (rp->any.link)
                    rp->any.link(rp, RESPROTO_LINK_DOWN);
            }
        }
    }

    return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult client_name_changed(DBusConnection *conn,
                                             DBusMessage    *msg,
                                             void           *user_data)
{
    (void)conn;
    (void)user_data;

    char              *sender;
    char              *before;
    char              *after;
    resproto_t        *rp;
    int                success;

    success = dbus_message_is_signal(msg, RESPROTO_DBUS_ADMIN_INTERFACE,
                                     RESPROTO_DBUS_NAME_OWNER_CHANGED_SIGNAL);

    if (!success)
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;


    success = dbus_message_get_args(msg, NULL,
                                    DBUS_TYPE_STRING, &sender,
                                    DBUS_TYPE_STRING, &before,
                                    DBUS_TYPE_STRING, &after,
                                    DBUS_TYPE_INVALID);
    
    if (success && sender && !strcmp(sender, RESPROTO_DBUS_MANAGER_NAME)) {
        if ((rp = find_resproto(conn)) != NULL) {

            if (after && strcmp(after, "")) {
                /* manager is up */
                if (rp->any.link)
                    rp->any.link(rp, RESPROTO_LINK_UP);
            }
            
            else if (before && (!after || !strcmp(after, ""))) {
                /* manager is gone */
                if (rp->any.link)
                    rp->any.link(rp, RESPROTO_LINK_DOWN);
            } 
        }
    }

    return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult manager_method(DBusConnection *conn,
                                        DBusMessage    *dbusmsg,
                                        void           *user_data)
{
    (void)user_data;

    int         type      = dbus_message_get_type(dbusmsg);
    const char *interface = dbus_message_get_interface(dbusmsg);
    const char *member    = dbus_message_get_member(dbusmsg);
    const char *sender    = dbus_message_get_sender(dbusmsg);
    resmsg_t    resmsg;
    resproto_t *rp;
    resset_t   *rset;
    char       *method;


    if (strcmp(interface, RESPROTO_DBUS_MANAGER_INTERFACE) ||
        type != DBUS_MESSAGE_TYPE_METHOD_CALL)
    {
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    if (resmsg_parse_dbus_message(dbusmsg, &resmsg) != NULL) {
        method = method_name(resmsg.type);

        if (method && !strcmp(method, member) && (rp = find_resproto(conn))) {
            for (rset = rp->any.rsets;   rset;   rset = rset->next) {

                if (!strcmp(sender, rset->peer) && resmsg.any.id == rset->id) {
                    if (resmsg.type != RESMSG_REGISTER) {
                        dbus_message_ref(dbusmsg);
                        rp->dbus.receive(&resmsg, rset, dbusmsg);
                    }
                        
                    return DBUS_HANDLER_RESULT_HANDLED;
                }
            }


            if (resmsg.type == RESMSG_REGISTER) {
                rset = resset_create(rp, sender, resmsg.any.id,
                                     RESPROTO_RSET_STATE_CONNECTED,
                                     resmsg.record.rset.all,
                                     resmsg.record.rset.share,
                                     resmsg.record.rset.opt);

                if (rset != NULL) {
                    dbus_message_ref(dbusmsg);
                    rp->dbus.receive(&resmsg, rset, dbusmsg);
                }
                    
                return DBUS_HANDLER_RESULT_HANDLED;
            }
        }
    }

    return DBUS_HANDLER_RESULT_HANDLED;
}

static char *method_name(resmsg_type_t msg_type)
{
    static char *method[RESMSG_MAX] = {
        [ RESMSG_REGISTER   ] = RESPROTO_DBUS_REGISTER_METHOD,
        [ RESMSG_UNREGISTER ] = RESPROTO_DBUS_UREGISTER_METHOD,
        [ RESMSG_UPDATE     ] = RESPROTO_DBUS_UPDATE_METHOD,
        [ RESMSG_ACQUIRE    ] = RESPROTO_DBUS_ACQUIRE_METHOD,
        [ RESMSG_RELEASE    ] = RESPROTO_DBUS_RELEASE_METHOD,
        [ RESMSG_GRANT      ] = RESPROTO_DBUS_GRANT_METHOD,
        [ RESMSG_ADVICE     ] = RESPROTO_DBUS_ADVICE_METHOD
    };

    char *retval = NULL;

    if (msg_type >= 0 && msg_type < RESMSG_MAX) {
        retval = method[msg_type];
    }
     
    return retval;
}



/* 
 * Local Variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim:set expandtab shiftwidth=4:
 */
