#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include "res-conn-private.h"
#include "res-set-private.h"
#include "dbus-proto.h"
#include "dbus-msg.h"


/* 
 * local function prototypes
 */
static resset_t *connect_to_manager(resconn_t *, resmsg_t*);
static resset_t *connect_fail(resconn_t *, resmsg_t *);
static void      disconnect_from_manager(resset_t *);
static int       send_message(resset_t *, resmsg_t *, resproto_status_t);
static int       send_error(resset_t *, resmsg_t *, void *);
static void      status_method(DBusPendingCall *, void *);

static resconn_t *find_resproto(DBusConnection *);

static int watch_manager(resconn_dbus_t *, int);
static int watch_client(resconn_dbus_t *, const char *, int);
static int remove_filter(resconn_dbus_t *, char *);
static int add_filter(resconn_dbus_t *, char *);
static int request_name(resconn_dbus_t *, char *);
static int register_manager_object(resconn_dbus_t *);
static int register_client_object(resconn_dbus_t *, uint32_t);
static int unregister_client_object(resconn_dbus_t *, uint32_t);

static DBusHandlerResult client_name_changed(DBusConnection *,
                                             DBusMessage *, void *);
static DBusHandlerResult manager_name_changed(DBusConnection *,
                                              DBusMessage *, void *);
static DBusHandlerResult manager_method(DBusConnection *,DBusMessage *,void *);
static DBusHandlerResult client_method(DBusConnection *,DBusMessage *,void *);
static char *method_name(resmsg_type_t);

/* 
 * local storage
 */
static int          timeout = -1;     /* message timeout in msec's */

int resproto_dbus_manager_init(resconn_dbus_t *rcon, va_list args)
{
    DBusConnection    *dcon  = va_arg(args, DBusConnection *);
    const char        *name  = dbus_bus_get_unique_name(dcon);

    int success = FALSE;

    rcon->conn  = dcon;

    if (dbus_connection_add_filter(dcon, manager_name_changed,NULL, NULL) &&
        request_name(rcon, RESPROTO_DBUS_MANAGER_NAME)                    &&
        register_manager_object(rcon)                                       )
    {
        rcon->connect = connect_fail;
        rcon->disconn = resset_destroy;
        rcon->send    = send_message;
        rcon->error   = send_error;
        rcon->dbusid  = strdup(name);
        rcon->path    = strdup(RESPROTO_DBUS_MANAGER_PATH);

        success = TRUE;
    }

    return success;
}



int resproto_dbus_client_init(resconn_dbus_t *rcon, va_list args)
{
    resconn_linkup_t   mgrup = va_arg(args, resconn_linkup_t);
    DBusConnection    *dcon  = va_arg(args, DBusConnection *);
    const char        *name  = dbus_bus_get_unique_name(dcon);
    int                success = FALSE;

    rcon->conn  = dcon;
    rcon->mgrup = mgrup;

    if (dbus_connection_add_filter(dcon, client_name_changed,NULL, NULL) &&
        watch_manager(rcon, TRUE)                                          )
    {    

        rcon->connect = connect_to_manager;
        rcon->disconn = disconnect_from_manager;
        rcon->send    = send_message;
        rcon->error   = send_error;
        rcon->dbusid  = strdup(name);
        rcon->path    = strdup("");
        
        success = TRUE;
    }

    return success;
}

static resset_t *connect_to_manager(resconn_t *rcon, resmsg_t *resmsg)
{
    char          *name  =  RESPROTO_DBUS_MANAGER_NAME;
    uint32_t       id    =  resmsg->record.id;
    resmsg_rset_t *flags = &resmsg->record.rset;
    const char    *class =  resmsg->record.class;
    resset_t      *rset;

    if ((rset = resset_find(rcon, name, id)) == NULL) {
        if (register_client_object(&rcon->dbus, id)) {
            rset = resset_create(rcon, name, id, RESPROTO_RSET_STATE_CREATED,
                                 class, flags->all, flags->share, flags->opt);
        }
    }

    return rset;
}

static resset_t *connect_fail(resconn_t *rcon, resmsg_t *resmsg)
{
    (void)rcon;
    (void)resmsg;

    return NULL;
}

static void disconnect_from_manager(resset_t *rset)
{
    unregister_client_object(&rset->resconn->dbus, rset->id);
    resset_destroy(rset);
}

static int send_message(resset_t *rset,resmsg_t *rmsg,resproto_status_t status)
{
    resconn_dbus_t  *rcon;
    DBusConnection  *dcon;
    DBusMessage     *dmsg;
    char            *dest;
    char            *path;
    char            *iface;
    char            *method;
    char             buf[1024];
    resmsg_type_t    type;
    uint32_t         serial;
    uint32_t         reqno;
    DBusPendingCall *pend;
    int              need_reply;
    resconn_reply_t *reply;
    int              success;

    if (!rset || !rmsg)
        return FALSE;

    rcon = &rset->resconn->dbus;
    dcon = rcon->conn;
    
    switch (rcon->role) {
        
    case RESPROTO_ROLE_MANAGER:
        snprintf(buf, sizeof(buf), RESPROTO_DBUS_CLIENT_PATH, rmsg->any.id);
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

    success = FALSE;

    if ((dest = rset->peer) && (method  = method_name(rmsg->type)) &&
        (dmsg = resmsg_dbus_compose_message(dest,path,iface,method,rmsg)))
    {
        if (rcon->role != RESPROTO_ROLE_CLIENT)
            need_reply = status ? TRUE : FALSE;
        else {
            switch (rmsg->any.type) {
            case RESMSG_REGISTER:    need_reply = TRUE;                  break;
            case RESMSG_UNREGISTER:  need_reply = TRUE;                  break;
            default:                 need_reply = status ? TRUE : FALSE; break;
            }
        }


        if (!need_reply)
            success = dbus_connection_send(dcon, dmsg, NULL);
        else {
            success = dbus_connection_send_with_reply(dcon,dmsg,&pend,timeout);

            if (success) {
                type    = rmsg->type;
                serial  = dbus_message_get_serial(dmsg);
                reqno   = rmsg->any.reqno;
                reply   = resconn_reply_create(type,serial,reqno,rset,status);

                success = dbus_pending_call_set_notify(pend,
                                                       status_method,
                                                       reply,
                                                       resconn_reply_destroy);
            }
        }

        if (success)
            resset_ref(rset);

        dbus_message_unref(dmsg);
    }
    
    return success;
}

static int send_error(resset_t *rset, resmsg_t *resreply, void *data)
{
    resconn_t      *rcon      = rset->resconn;
    DBusConnection *dcon      = rcon->dbus.conn; 
    DBusMessage    *dbusmsg   = (DBusMessage *)data;
    dbus_uint32_t   serial    = dbus_message_get_serial(dbusmsg);
    DBusMessage    *dbusreply = resmsg_dbus_reply_message(dbusmsg, resreply);

    dbus_connection_send(dcon, dbusreply, &serial);
    dbus_message_unref(dbusreply);
    dbus_message_unref(dbusmsg);

    return TRUE;
}

static void status_method(DBusPendingCall *pend, void *data)
{
    resconn_reply_t *reply   = (resconn_reply_t *)data;
    DBusMessage     *dbusmsg = dbus_pending_call_steal_reply(pend);
    resset_t        *rset;
    resconn_t       *rcon;
    resmsg_t         resmsg;
    const char      *errmsg;

    if (reply && dbusmsg){
        rset = reply->rset;
        rcon = rset->resconn;
        
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
            if (!resmsg_dbus_parse_message(dbusmsg, &resmsg)            ||
                reply->serial != dbus_message_get_reply_serial(dbusmsg) ||
                resmsg.status.type  != RESMSG_STATUS                    ||
                resmsg.status.id    != rset->id                         ||
                resmsg.status.reqno != reply->reqno                       )
            {
                printf("serial(%u,%u) type(%d,%d) id(%u,%u) reqno(%u,%u)\n",
                       reply->serial, dbus_message_get_reply_serial(dbusmsg),
                       resmsg.status.type, RESMSG_STATUS, 
                       resmsg.status.id, rset->id,
                       resmsg.status.reqno, reply->reqno);

                memset(&resmsg, 0, sizeof(resmsg));
                resmsg.status.type   = RESMSG_STATUS;
                resmsg.status.id     = rset->id;
                resmsg.status.reqno  = reply->reqno;
                resmsg.status.errcod = -1;
                resmsg.status.errmsg = "<peer error>";
            }
        }

        if (rcon->any.role == RESPROTO_ROLE_CLIENT) {
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


static resconn_t *find_resproto(DBusConnection *dcon)
{
    resconn_t *rcon = NULL;

    while ((rcon = resconn_list_iterate(rcon)) != NULL) {
        if (rcon->any.transp == RESPROTO_TRANSPORT_DBUS &&
            rcon->dbus.conn  == dcon                      )
        {
            break;
        }
    }
    
    return rcon;
}

static int watch_manager(resconn_dbus_t *rcon, int watchit)
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
        success = add_filter(rcon, filter);
    else
        success = remove_filter(rcon, filter);
    
    return success;
}


static int watch_client(resconn_dbus_t *rcon, const char *dbusid, int watchit)
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
        success = add_filter(rcon, filter);
    else
        success = remove_filter(rcon, filter);
    
    return success;
}


static int add_filter(resconn_dbus_t *rcon, char *filter)
{
    DBusError  err;

    dbus_error_init(&err);
    dbus_bus_add_match(rcon->conn, filter, &err);

    if (dbus_error_is_set(&err)) {
        dbus_error_free(&err);
        return FALSE;
    }

    return TRUE;
}

static int remove_filter(resconn_dbus_t *rcon, char *filter)
{
    dbus_bus_remove_match(rcon->conn, filter, NULL);

    return TRUE;
}

static int request_name(resconn_dbus_t *rcon, char *name)
{
    DBusError  err;
    int        retval;
    int        success;

    dbus_error_init(&err);

    retval = dbus_bus_request_name(rcon->conn, name,
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

static int register_manager_object(resconn_dbus_t *rcon)
{
    static struct DBusObjectPathVTable method = {
        .message_function = manager_method
    };

    int success;

    success = dbus_connection_register_object_path(rcon->conn,
                                                   RESPROTO_DBUS_MANAGER_PATH,
                                                   &method, NULL);
    
    return success;
}


static int register_client_object(resconn_dbus_t *rcon, uint32_t id)
{
    static struct DBusObjectPathVTable method = {
        .message_function = client_method
    };

    char path[1024];
    int  success;

    snprintf(path, sizeof(path), RESPROTO_DBUS_CLIENT_PATH, id);

    success = dbus_connection_register_object_path(rcon->conn, path,
                                                   &method, NULL);

    if (success) {
        free(rcon->path);
        rcon->path = strdup(path);
    }
    
    return success;
}

static int unregister_client_object(resconn_dbus_t *rcon, uint32_t id)
{
    char path[1024];
    int  success;

    snprintf(path, sizeof(path), RESPROTO_DBUS_CLIENT_PATH, id);

    return  dbus_connection_unregister_object_path(rcon->conn, path);
}



static DBusHandlerResult manager_name_changed(DBusConnection *dcon,
                                              DBusMessage    *msg,
                                              void           *user_data)
{
    (void)user_data;

    char              *sender;
    char              *before;
    char              *after;
    resconn_t         *rcon;
    int                success;
    DBusHandlerResult  result;
  
    result  = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    success = dbus_message_is_signal(msg, RESPROTO_DBUS_ADMIN_INTERFACE,
                                     RESPROTO_DBUS_NAME_OWNER_CHANGED_SIGNAL);


    if (success) {
        success = dbus_message_get_args(msg, NULL,
                                        DBUS_TYPE_STRING, &sender,
                                        DBUS_TYPE_STRING, &before,
                                        DBUS_TYPE_STRING, &after,
                                        DBUS_TYPE_INVALID);
        
        if (success && sender != NULL && before != NULL) {

            if ((rcon = find_resproto(dcon)) != NULL) {
                                
                if (!after || !strcmp(after, "")) {
                    /* client is gone */
                    
                    if (rcon->any.link) {
                        if (rcon->any.link(rcon, sender, RESPROTO_LINK_DOWN)) {
                            watch_client(&rcon->dbus, sender, FALSE);
                            result = DBUS_HANDLER_RESULT_HANDLED;
                        }
                    }
                }
            }    
        }
    }
     
    return result;
}

static DBusHandlerResult client_name_changed(DBusConnection *dcon,
                                             DBusMessage    *msg,
                                             void           *user_data)
{
    (void)user_data;

    char      *sender;
    char      *before;
    char      *after;
    resconn_t *rcon;
    int        success;

    success = dbus_message_is_signal(msg, RESPROTO_DBUS_ADMIN_INTERFACE,
                                     RESPROTO_DBUS_NAME_OWNER_CHANGED_SIGNAL);

    if (success) {
        success = dbus_message_get_args(msg, NULL,
                                        DBUS_TYPE_STRING, &sender,
                                        DBUS_TYPE_STRING, &before,
                                        DBUS_TYPE_STRING, &after,
                                        DBUS_TYPE_INVALID);
    
        if (success && sender && !strcmp(sender, RESPROTO_DBUS_MANAGER_NAME)) {
            if ((rcon = find_resproto(dcon)) != NULL) {
                
                if (after && strcmp(after, "")) {
                    /* manager is up */
                    if (rcon->any.link)
                        rcon->any.link(rcon, after, RESPROTO_LINK_UP);
                }
                
                else if (before && (!after || !strcmp(after, ""))) {
                    /* manager is gone */
                    if (rcon->any.link)
                        rcon->any.link(rcon, before, RESPROTO_LINK_DOWN);
                } 
            }
        }

        return DBUS_HANDLER_RESULT_HANDLED;
    }

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static DBusHandlerResult manager_method(DBusConnection *dcon,
                                        DBusMessage    *dbusmsg,
                                        void           *user_data)
{
    (void)user_data;

    int         type      = dbus_message_get_type(dbusmsg);
    const char *interface = dbus_message_get_interface(dbusmsg);
    const char *member    = dbus_message_get_member(dbusmsg);
    const char *sender    = dbus_message_get_sender(dbusmsg);
    resmsg_t    resmsg;
    resconn_t  *rcon;
    resset_t   *rset;
    char       *method;


    if (!strcmp(interface, RESPROTO_DBUS_MANAGER_INTERFACE) &&
        type == DBUS_MESSAGE_TYPE_METHOD_CALL               &&
        resmsg_dbus_parse_message(dbusmsg, &resmsg) != NULL   )
    {
        method = method_name(resmsg.type);

        if (method && !strcmp(method,member) && (rcon = find_resproto(dcon))) {
            for (rset = rcon->any.rsets;   rset;   rset = rset->next) {

                if (!strcmp(sender, rset->peer) && resmsg.any.id == rset->id) {
                    if (resmsg.type != RESMSG_REGISTER) {
                        dbus_message_ref(dbusmsg);
                        rcon->dbus.receive(&resmsg, rset, dbusmsg);
                    }
                        
                    return DBUS_HANDLER_RESULT_HANDLED;
                }
            }


            if (resmsg.type == RESMSG_REGISTER) {
                rset = resset_create(rcon, sender, resmsg.any.id,
                                     RESPROTO_RSET_STATE_CONNECTED,
                                     resmsg.record.class,
                                     resmsg.record.rset.all,
                                     resmsg.record.rset.share,
                                     resmsg.record.rset.opt);

                if (rset != NULL && watch_client(&rcon->dbus, sender, TRUE)) {
                    dbus_message_ref(dbusmsg);
                    rcon->dbus.receive(&resmsg, rset, dbusmsg);
                }
                    
                return DBUS_HANDLER_RESULT_HANDLED;
            }
        }
    }

    return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult client_method(DBusConnection *dcon,
                                       DBusMessage    *dbusmsg,
                                       void           *user_data)
{
    (void)user_data;

    int         type      = dbus_message_get_type(dbusmsg);
    const char *interface = dbus_message_get_interface(dbusmsg);
    const char *member    = dbus_message_get_member(dbusmsg);
    char       *name      =  RESPROTO_DBUS_MANAGER_NAME;
    resmsg_t    resmsg;
    resconn_t  *rcon;
    resset_t   *rset;
    char       *method;

    if (!strcmp(interface, RESPROTO_DBUS_CLIENT_INTERFACE)  &&
        type == DBUS_MESSAGE_TYPE_METHOD_CALL               &&
        resmsg_dbus_parse_message(dbusmsg, &resmsg) != NULL   )
    {
        method = method_name(resmsg.type);

        if (method && !strcmp(method,member) && (rcon = find_resproto(dcon))) {
            for (rset = rcon->any.rsets;   rset;   rset = rset->next) {
                if (!strcmp(name, rset->peer) && resmsg.any.id == rset->id) {
                    dbus_message_ref(dbusmsg);
                    rcon->dbus.receive(&resmsg, rset, dbusmsg);
                    return DBUS_HANDLER_RESULT_HANDLED;
                }
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
