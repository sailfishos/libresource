#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "resproto.h"
#include "resmsg.h"

static char *flag_str(uint32_t);


DBusMessage *resmsg_compose_dbus_message(const char *dest,
                                         const char *path,
                                         const char *interface,
                                         const char *method,
                                         resmsg_t   *resmsg)
{
    static char      *empty_str = "";

    DBusMessage      *dbusmsg = NULL;
    resmsg_record_t  *record;
    resmsg_possess_t *possess;
    resmsg_notify_t  *notify;
    resmsg_status_t  *status;
    int               success;

    if (!dest || !path || !interface || !method || !resmsg)
        goto compose_error;

    dbusmsg = dbus_message_new_method_call(dest, path, interface, method);

    if (dbusmsg == NULL)
        goto compose_error;

    switch (resmsg->type) {

    default:
        success = FALSE;
        break;

    case RESMSG_REGISTER:
    case RESMSG_UPDATE:
        record  = &resmsg->record;
        success = dbus_message_append_args(dbusmsg,
                                 DBUS_TYPE_INT32 , &record->type,
                                 DBUS_TYPE_UINT32, &record->id,
                                 DBUS_TYPE_UINT32, &record->reqno,
                                 DBUS_TYPE_UINT32, &record->rset.all,
                                 DBUS_TYPE_UINT32, &record->rset.share,
                                 DBUS_TYPE_UINT32, &record->rset.opt,
                                 DBUS_TYPE_STRING,  record->class ?
                                                   &record->class : &empty_str,
                                 DBUS_TYPE_INVALID);
        break;

    case RESMSG_UNREGISTER:
    case RESMSG_ACQUIRE:
    case RESMSG_RELEASE:
        possess = &resmsg->possess;
        success = dbus_message_append_args(dbusmsg,
                                           DBUS_TYPE_INT32 , &possess->type,
                                           DBUS_TYPE_UINT32, &possess->id,
                                           DBUS_TYPE_UINT32, &possess->reqno,
                                           DBUS_TYPE_INVALID);
        break;

    case RESMSG_GRANT:
    case RESMSG_ADVICE:
        notify  = &resmsg->notify;
        success = dbus_message_append_args(dbusmsg,
                                           DBUS_TYPE_INT32 , &notify->type,
                                           DBUS_TYPE_UINT32, &notify->id,
                                           DBUS_TYPE_UINT32, &notify->reqno,
                                           DBUS_TYPE_UINT32, &notify->resrc,
                                           DBUS_TYPE_INVALID);
        break;
    }

    if (!success)
        goto compose_error;

    return dbusmsg;

 compose_error:
    if (dbusmsg != NULL)
        dbus_message_unref(dbusmsg);

    return NULL;
}

DBusMessage *resmsg_reply_dbus_message(DBusMessage *dbusmsg,resmsg_t *resreply)
{
    static const char *empty_str = "";

    DBusMessage       *dbusreply;
    resmsg_status_t   *status;
    dbus_uint32_t      serial;
    int                success;

    if (!dbusmsg || !resreply || resreply->type != RESMSG_STATUS)
        return NULL;
    

    status    = &resreply->status;
    dbusreply = dbus_message_new_method_return(dbusmsg);
    success   = dbus_message_append_args(dbusreply,
                                         DBUS_TYPE_INT32 , &status->type,
                                         DBUS_TYPE_UINT32, &status->id,
                                         DBUS_TYPE_UINT32, &status->reqno,
                                         DBUS_TYPE_INT32 , &status->errcod,
                                         DBUS_TYPE_STRING,  status->errmsg ?
                                                 &status->errmsg : &empty_str,
                                         DBUS_TYPE_INVALID);
    if (!success) {
        dbus_message_unref(dbusreply);
        dbusmsg = NULL;
    }

    return dbusreply;
}

resmsg_t *resmsg_parse_dbus_message(DBusMessage *dbusmsg, resmsg_t *resmsg)
{
    int32_t           type;
    resmsg_record_t  *record;
    resmsg_possess_t *possess;
    resmsg_notify_t  *notify;
    resmsg_status_t  *status;
    int               free_resmsg;
    int               success;
    
    if (dbusmsg == NULL) {
        free_resmsg = FALSE;
        goto parse_error;
    }

    /*
     * make sure we have a valid structure to populate with data
     */
    if (resmsg != NULL)
        free_resmsg = FALSE;
    else {
        free_resmsg = TRUE;
        resmsg = malloc(sizeof(resmsg_t));
    }

    if (resmsg == NULL)
        goto parse_error;

    memset(resmsg, 0, sizeof(resmsg_t));


    /*
     * first get the type
     */
    success = dbus_message_get_args(dbusmsg, NULL,
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
    case RESMSG_UPDATE:
        record  = &resmsg->record;
        success = dbus_message_get_args(dbusmsg, NULL,
                                        DBUS_TYPE_INT32 , &record->type,
                                        DBUS_TYPE_UINT32, &record->id,
                                        DBUS_TYPE_UINT32, &record->reqno,
                                        DBUS_TYPE_UINT32, &record->rset.all,
                                        DBUS_TYPE_UINT32, &record->rset.share,
                                        DBUS_TYPE_UINT32, &record->rset.opt,
                                        DBUS_TYPE_STRING, &record->class,
                                        DBUS_TYPE_INVALID);
        break;

    case RESMSG_UNREGISTER:
    case RESMSG_ACQUIRE:
    case RESMSG_RELEASE:
        possess = &resmsg->possess;
        success = dbus_message_get_args(dbusmsg, NULL,
                                        DBUS_TYPE_INT32 , &possess->type,
                                        DBUS_TYPE_UINT32, &possess->id,
                                        DBUS_TYPE_UINT32, &possess->reqno,
                                        DBUS_TYPE_INVALID);
        break;


    case RESMSG_GRANT:
    case RESMSG_ADVICE:
        notify  = &resmsg->notify;
        success = dbus_message_get_args(dbusmsg, NULL,
                                        DBUS_TYPE_INT32 , &notify->type,
                                        DBUS_TYPE_UINT32, &notify->id,
                                        DBUS_TYPE_UINT32, &notify->reqno,
                                        DBUS_TYPE_UINT32, &notify->resrc,
                                        DBUS_TYPE_INVALID);
        break;

    case RESMSG_STATUS:
        status  = &resmsg->status;
        success = dbus_message_get_args(dbusmsg, NULL,
                                        DBUS_TYPE_INT32 , &status->type,
                                        DBUS_TYPE_UINT32, &status->id,
                                        DBUS_TYPE_UINT32, &status->reqno,
                                        DBUS_TYPE_INT32 , &status->errcod,
                                        DBUS_TYPE_STRING, &status->errmsg,
                                        DBUS_TYPE_INVALID);
        break;
    }
        
    if (!success)
        goto parse_error;

    /* everything looks OK */
    return resmsg;

    /* something went wrong */
 parse_error:
    if (resmsg != NULL && free_resmsg)
        free(resmsg);

    return NULL;
}


resmsg_t *resmsg_copy_internal_message(resmsg_t *src)
{
    resmsg_t *dst = NULL;

    if (src != NULL && (dst = malloc(sizeof(resmsg_t))) != NULL) {
        memset(dst, 0, sizeof(resmsg_t));
        
        switch (src->type) {

        case RESMSG_REGISTER:
        case RESMSG_UPDATE:
            dst->record = src->record;
            dst->record.class = strdup(src->record.class);
            break;

        case RESMSG_UNREGISTER:
        case RESMSG_ACQUIRE:
        case RESMSG_RELEASE:
            dst->possess = src->possess;
            break;

        case RESMSG_GRANT:
        case RESMSG_ADVICE:
            dst->notify = src->notify;
            break;

        case RESMSG_STATUS:
            dst->status = src->status;
            dst->status.errmsg = strdup(src->status.errmsg);
            break;

        default:
            free(dst);
            dst = NULL;
            break;
        }
    }

    return dst;
}

void resmsg_destroy_internal_message(resmsg_t *msg)
{
    if (msg != NULL) {
        switch (msg->type) {

        case RESMSG_REGISTER:
        case RESMSG_UPDATE:
            free(msg->record.class);
            break;

        case RESMSG_STATUS:
            free((void *)msg->status.errmsg);
            break;

        default:
            break;
        }

        free(msg);
    }
}

char *resmsg_dump_message(resmsg_t *resmsg, int indent, char *buf, int len)
{
#define PRINT(fmt, args...)                                              \
    do {                                                                 \
        if (len > 0) {                                                   \
            p += (l = snprintf(p, len, "%s" fmt "\n", spaces, ##args));  \
            l -= len;                                                    \
        }                                                                \
    } while(0)

    char  spaces[256];
    int   l;
    char *p;
    char  r[512];
    resmsg_rset_t    *rset;
    resmsg_record_t  *record;
    resmsg_possess_t *possess;
    resmsg_notify_t  *notify;
    resmsg_status_t  *status;

    if (!buf || len < 1 || indent < 0)
        return "";

    p = buf;
    *buf = '\0';
    memset(spaces, ' ', sizeof(spaces));
    spaces[indent < sizeof(spaces) ? indent : sizeof(spaces)-1] = '\0';

    PRINT("type      : %s (%d)",  resmsg_type_str(resmsg->type), resmsg->type);
    PRINT("id        : %u"     ,  resmsg->any.id);
    PRINT("reqno     : %u"     ,  resmsg->any.reqno);

    switch (resmsg->type) {

    case RESMSG_REGISTER:
    case RESMSG_UPDATE:
        record = &resmsg->record;
        rset   = &record->rset;
        PRINT("rset.all  : %s"  , resmsg_res_str(rset->all  , r, sizeof(r)));
        PRINT("rset.share: %s"  , resmsg_res_str(rset->share, r, sizeof(r)));
        PRINT("rset.opt  : %s"  , resmsg_res_str(rset->opt  , r, sizeof(r)));
        PRINT("class     : '%s'", record->class && record->class[0] ?
                                         record->class : "<unknown>");
        break;

    case RESMSG_UNREGISTER:
    case RESMSG_ACQUIRE:
    case RESMSG_RELEASE:
        possess = &resmsg->possess;
        break;

    case RESMSG_GRANT:
    case RESMSG_ADVICE:
        notify = &resmsg->notify;
        PRINT("resrc     : %s", resmsg_res_str(notify->resrc, r, sizeof(r)));
        break;

    case RESMSG_STATUS:
        status = &resmsg->status;
        PRINT("errcod    : %d"  , status->errcod);
        PRINT("errstr    : '%s'", status->errmsg);        
        break;

    default:
        break;
    }

    return buf;

#undef PRINT
}

char *resmsg_type_str(resmsg_type_t type)
{
    char *str;

    switch (type) {
    case RESMSG_REGISTER:      str = "register";         break;
    case RESMSG_UNREGISTER:    str = "unregister";       break;
    case RESMSG_UPDATE:        str = "update";           break;
    case RESMSG_ACQUIRE:       str = "acquire";          break;
    case RESMSG_RELEASE:       str = "releaase";         break;
    case RESMSG_GRANT:         str = "grant";            break;
    case RESMSG_ADVICE:        str = "advice";           break;
    case RESMSG_STATUS:        str = "status";           break;
    default:                   str = "<unknown type>";   break;
    }

    return str;
}



char *resmsg_res_str(uint32_t res, char *buf, int len)
{
    char    *p;
    char    *s;
    char    *f;
    int      l;
    uint32_t m;
    uint32_t i;
    char     hex[64];

    if (!buf || len < 1)
        return "";

    *buf = '\0';
    
    snprintf(hex, sizeof(hex), "0x%x", res);
    
    for (p = buf, s = "", i = 0;   i < 32 && res != 0 && len > 0;   i++) {
        m = 1 << i;
        
        if ((res & m) != 0) {
            res &= ~m;
            
            if ((f = flag_str(m)) != NULL) {
                l = snprintf(p, len, "%s%s", s, f);
                s = " | ";
                
                p += l;
                len -= l;
            }
        }
    } /* for */
    
    if (l > 0)
        snprintf(p, len, "%s(%s)", *s ? " ":"", hex);

    return buf;
}

static char *flag_str(uint32_t flag)
{
    char *str;

    switch (flag) {
    case RESMSG_AUDIO_PLAYBACK:     str = "audio_playback";    break;
    case RESMSG_VIDEO_PLAYBACK:     str = "video_playback";    break;
    case RESMSG_AUDIO_RECORDING:    str = "audio_recording";   break;
    case RESMSG_VIDEO_RECORDING:    str = "video_recording";   break;
    default:                        str = NULL;                break;
    }

    return str;
}




/* 
 * Local Variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim:set expandtab shiftwidth=4:
 */
