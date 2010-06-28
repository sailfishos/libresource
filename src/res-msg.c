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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "res-conn-private.h"

#include "visibility.h"

static char *flag_str(uint32_t);
static char *mode_str(uint32_t);


EXPORT char *resmsg_dump_message(resmsg_t *resmsg,
                                 int       indent,
                                 char     *buf,
                                 int       len)
{
#define PRINT(fmt, args...)                                              \
    do {                                                                 \
        if (len > 0) {                                                   \
            p += (l = snprintf(p, len, "%s" fmt "\n", spaces, ##args));  \
            len -= l;                                                    \
        }                                                                \
    } while(0)

    char  spaces[256];
    int   l;
    char *p;
    char  r[512];
    char  m[512];
    resmsg_rset_t     *rset;
    resmsg_record_t   *record;
    resmsg_possess_t  *possess;
    resmsg_notify_t   *notify;
    resmsg_audio_t    *audio;
    resmsg_status_t   *status;
    resmsg_property_t *property;
    resmsg_match_t    *match;

    if (!buf || len < 1 || indent < 0)
        return "";

    p = buf;
    *buf = '\0';
    memset(spaces, ' ', sizeof(spaces));
    spaces[indent < sizeof(spaces) ? indent : sizeof(spaces)-1] = '\0';

    PRINT("type       : %s (%d)",  resmsg_type_str(resmsg->type),resmsg->type);
    PRINT("id         : %u"     ,  resmsg->any.id);
    PRINT("reqno      : %u"     ,  resmsg->any.reqno);

    switch (resmsg->type) {

    case RESMSG_REGISTER:
    case RESMSG_UPDATE:
        record = &resmsg->record;
        rset   = &record->rset;
        PRINT("rset.all   : %s"  , resmsg_res_str(rset->all  ,  r, sizeof(r)));
        PRINT("rset.opt   : %s"  , resmsg_res_str(rset->opt  ,  r, sizeof(r)));
        PRINT("rset.share : %s"  , resmsg_res_str(rset->share,  r, sizeof(r)));
        PRINT("rset.mask  : %s"  , resmsg_res_str(rset->mask ,  r, sizeof(r)));
        PRINT("klass      : '%s'", record->klass && record->klass[0] ?
                                        record->klass : "<unknown>");
        PRINT("mode       : %s"  , resmsg_mod_str(record->mode, m, sizeof(m)));
        break;

    case RESMSG_UNREGISTER:
    case RESMSG_ACQUIRE:
    case RESMSG_RELEASE:
        possess = &resmsg->possess;
        break;

    case RESMSG_GRANT:
    case RESMSG_ADVICE:
        notify = &resmsg->notify;
        PRINT("resrc      : %s", resmsg_res_str(notify->resrc, r, sizeof(r)));
        break;

    case RESMSG_AUDIO:
        audio    = &resmsg->audio;
        property = &audio->property;
        match    = &property->match;
        PRINT("group      : '%s'", audio->group);
        PRINT("pid        : %u"  , audio->pid);
        PRINT("property   :");
        PRINT("  name     : '%s'", property->name);
        PRINT("  match    :");
        PRINT("    method : %s"  , resmsg_match_method_str(match->method));
        PRINT("    pattern: '%s'", match->pattern);
        break;

    case RESMSG_STATUS:
        status = &resmsg->status;
        PRINT("errcod    : %d"  , status->errcod);
        PRINT("errstr    : '%s'", status->errmsg);        
        break;

    default:
        break;
    }

    /* remove the last newline, if any */
    if (--p > buf && *p == '\n')
        *p = '\0';

    return buf;

#undef PRINT
}

EXPORT char *resmsg_type_str(resmsg_type_t type)
{
    char *str;

    switch (type) {
    case RESMSG_REGISTER:      str = "register";         break;
    case RESMSG_UNREGISTER:    str = "unregister";       break;
    case RESMSG_UPDATE:        str = "update";           break;
    case RESMSG_ACQUIRE:       str = "acquire";          break;
    case RESMSG_RELEASE:       str = "release";          break;
    case RESMSG_GRANT:         str = "grant";            break;
    case RESMSG_ADVICE:        str = "advice";           break;
    case RESMSG_AUDIO:         str = "audio";            break;
    case RESMSG_STATUS:        str = "status";           break;
    default:                   str = "<unknown type>";   break;
    }

    return str;
}


EXPORT char *resmsg_mod_str(uint32_t mod, char *buf, int len)
{
    char    *p;
    char    *s;
    char    *mstr;
    int      l = len;
    uint32_t m;
    uint32_t i;
    char     hex[64];

    if (!buf || len < 1)
        return "";

    *buf = '\0';
    
    snprintf(hex, sizeof(hex), "0x%x", mod);
    
    for (p = buf, s = "", i = 0;   i < 32 && mod != 0 && len > 0;   i++) {
        m = 1 << i;
        
        if ((mod & m) != 0) {
            mod &= ~m;
            
            if ((mstr = mode_str(m)) != NULL) {
                l = snprintf(p, len, "%s%s", s, mstr);
                s = ",";
                
                p += l;
                len -= l;
            }
        }
    } /* for */
    
    if (l > 0)
        snprintf(p, len, "%s(%s)", *s ? " ":"<no-mode> ", hex);

    return buf;
}


EXPORT char *resmsg_res_str(uint32_t res, char *buf, int len)
{
    char    *p;
    char    *s;
    char    *f;
    int      l = len;
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
                s = ",";
                
                p += l;
                len -= l;
            }
        }
    } /* for */
    
    if (l > 0)
        snprintf(p, len, "%s(%s)", *s ? " ":"<no-resource> ", hex);

    return buf;
}

EXPORT char *resmsg_match_method_str(resmsg_match_method_t method)
{
    char *str;

    switch (method) {
    case resmsg_method_equals:      str = "equals";      break;
    case resmsg_method_startswith:  str = "startswith";  break;
    case resmsg_method_matches:     str = "matches";     break;
    default:                        str = "<unknown>";   break;
    }

    return str;
}

static char *flag_str(uint32_t flag)
{
    char *str;

    switch (flag) {
    case RESMSG_AUDIO_PLAYBACK:     str = "audio_playback";    break;
    case RESMSG_VIDEO_PLAYBACK:     str = "video_playback";    break;
    case RESMSG_AUDIO_RECORDING:    str = "audio_recording";   break;
    case RESMSG_VIDEO_RECORDING:    str = "video_recording";   break;
    case RESMSG_VIBRA:              str = "vibra";             break;
    case RESMSG_LEDS:               str = "leds";              break;
    case RESMSG_BACKLIGHT:          str = "backlight";         break;
    case RESMSG_SYSTEM_BUTTON:      str = "system_button";     break;
    case RESMSG_LOCK_BUTTON:        str = "lock_button";       break;
    case RESMSG_SCALE_BUTTON:       str = "scale_button";      break;
    case RESMSG_SNAP_BUTTON:        str = "snap_button";       break;
    case RESMSG_LENS_COVER:         str = "lens_cover";        break;
    case RESMSG_HEADSET_BUTTONS:    str = "lens_cover";        break;
    default:                        str = NULL;                break;
    }

    return str;
}


static char *mode_str(uint32_t mode_bit)
{
    char *str;

    switch (mode_bit) {
    case RESMSG_MODE_AUTO_RELEASE:  str = "AutoRelease";   break;
    case RESMSG_MODE_ALWAYS_REPLY:  str = "AlwaysReply";   break;
    default:                        str = NULL;            break;
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
