#include <stdlib.h>
#include <string.h>

#include "resproto.h"
#include "resmsg.h"

DBusMessage *resmsg_compose_dbus_message(const char *dest,
                                         const char *path,
                                         const char *interface,
                                         const char *method,
                                         resmsg_t   *res_msg)
{
    static char      *empty_str = "";

    DBusMessage      *dbus_msg = NULL;
    resmsg_record_t  *record;
    resmsg_possess_t *possess;
    resmsg_notify_t  *notify;
    int               success;

    if (!dest || !path || !interface || !method || !res_msg)
        goto compose_error;

    dbus_msg = dbus_message_new_method_call(dest, path, interface, method);

    if (dbus_msg == NULL)
        goto compose_error;

    switch (res_msg->type) {

    default:
        success = FALSE;
        break;

    case RESMSG_REGISTER:
    case RESMSG_UNREGISTER:
    case RESMSG_UPDATE:
        record  = &res_msg->record;
        success = dbus_message_append_args(dbus_msg,
                                 DBUS_TYPE_INT32 , &record->type,
                                 DBUS_TYPE_UINT32, &record->id,
                                 DBUS_TYPE_UINT32, &record->rset.all,
                                 DBUS_TYPE_UINT32, &record->rset.share,
                                 DBUS_TYPE_UINT32, &record->rset.opt,
                                 DBUS_TYPE_STRING,  record->class ?
                                                   &record->class : &empty_str,
                                 DBUS_TYPE_INVALID);
        break;

    case RESMSG_ACQUIRE:
    case RESMSG_RELEASE:
        possess = &res_msg->possess;
        success = dbus_message_append_args(dbus_msg,
                                           DBUS_TYPE_INT32 , &possess->type,
                                           DBUS_TYPE_UINT32, &possess->id,
                                           DBUS_TYPE_UINT32, &possess->reqno,
                                           DBUS_TYPE_INVALID);
        break;

    case RESMSG_GRANT:
    case RESMSG_ADVICE:
        notify  = &res_msg->notify;
        success = dbus_message_append_args(dbus_msg,
                                           DBUS_TYPE_INT32 , &notify->type,
                                           DBUS_TYPE_UINT32, &notify->id,
                                           DBUS_TYPE_UINT32, &notify->resrc,
                                           DBUS_TYPE_UINT32, &notify->reqno,
                                           DBUS_TYPE_INVALID);
        break;
    }

    if (!success)
        goto compose_error;

    return dbus_msg;

 compose_error:
    if (dbus_msg != NULL)
        dbus_message_unref(dbus_msg);

    return NULL;
}

resmsg_t *resmsg_parse_dbus_message(DBusMessage *dbus_msg, resmsg_t *res_msg)
{
    int32_t           type;
    resmsg_record_t  *record;
    resmsg_possess_t *possess;
    resmsg_notify_t  *notify;
    int               free_res_msg;
    int               success;
    
    if (dbus_msg == NULL) {
        free_res_msg = FALSE;
        goto parse_error;
    }

    /*
     * make sure we have a valid structure to populate with data
     */
    if (res_msg != NULL)
        free_res_msg = FALSE;
    else {
        free_res_msg = TRUE;
        res_msg = malloc(sizeof(resmsg_t));
    }

    if (res_msg == NULL)
        goto parse_error;

    memset(res_msg, 0, sizeof(resmsg_t));


    /*
     * first get the type
     */
    success = dbus_message_get_args(dbus_msg, NULL,
                                    DBUS_TYPE_INT32, &type,
                                    DBUS_TYPE_INVALID);
    if (!success)
        goto parse_error;


    /*
     * parse the whole message
     */
    switch (type) {

    default:
        success = FALSE;
        type    = RESMSG_INVALID;
        break;

    case RESMSG_REGISTER:
    case RESMSG_UNREGISTER:
    case RESMSG_UPDATE:
        record  = &res_msg->record;
        success = dbus_message_get_args(dbus_msg, NULL,
                                        DBUS_TYPE_INT32 , &record->type,
                                        DBUS_TYPE_UINT32, &record->id,
                                        DBUS_TYPE_UINT32, &record->rset.all,
                                        DBUS_TYPE_UINT32, &record->rset.share,
                                        DBUS_TYPE_UINT32, &record->rset.opt,
                                        DBUS_TYPE_STRING, &record->class,
                                        DBUS_TYPE_INVALID);
        break;

    case RESMSG_ACQUIRE:
    case RESMSG_RELEASE:
        possess = &res_msg->possess;
        success = dbus_message_get_args(dbus_msg, NULL,
                                        DBUS_TYPE_INT32 , &possess->type,
                                        DBUS_TYPE_UINT32, &possess->id,
                                        DBUS_TYPE_UINT32, &possess->reqno,
                                        DBUS_TYPE_INVALID);
        break;


    case RESMSG_GRANT:
    case RESMSG_ADVICE:
        notify  = &res_msg->notify;
        success = dbus_message_get_args(dbus_msg, NULL,
                                        DBUS_TYPE_INT32 , &notify->type,
                                        DBUS_TYPE_UINT32, &notify->id,
                                        DBUS_TYPE_UINT32, &notify->resrc,
                                        DBUS_TYPE_UINT32, &notify->reqno,
                                        DBUS_TYPE_INVALID);
        break;
    }
        
    if (!success)
        goto parse_error;

    /* everything looks OK */
    return res_msg;

    /* something went wrong */
 parse_error:
    if (res_msg != NULL && free_res_msg)
        free(res_msg);

    return NULL;
}



/* 
 * Local Variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim:set expandtab shiftwidth=4:
 */
