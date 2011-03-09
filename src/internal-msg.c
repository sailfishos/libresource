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

#include "res-msg.h"
#include "internal-msg.h"

resmsg_t *resmsg_internal_copy_message(resmsg_t *src)
{
    resmsg_t *dst = NULL;
    resmsg_property_t *src_prop, *dst_prop;
    resmsg_match_t    *src_match, *dst_match;

    if (src != NULL && (dst = malloc(sizeof(resmsg_t))) != NULL) {
        memset(dst, 0, sizeof(resmsg_t));
        
        switch (src->type) {

        case RESMSG_REGISTER:
        case RESMSG_UPDATE:
            dst->record = src->record;
            dst->record.klass = strdup(src->record.klass);
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

        case RESMSG_AUDIO:
            src_prop  = &src->audio.property;
            dst_prop  = &dst->audio.property;
            src_match = &src_prop->match;
            dst_match = &dst_prop->match;

            dst->audio         = src->audio;
            dst->audio.group   = strdup(src->audio.group);
            dst_prop->name     = strdup(src_prop->name);
            dst_match->pattern = strdup(src_match->pattern);
            break;

        case RESMSG_VIDEO:
            dst->video = src->video;
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

void resmsg_internal_destroy_message(resmsg_t *msg)
{
    resmsg_property_t *prop;
    resmsg_match_t    *match;

    if (msg != NULL) {
        switch (msg->type) {

        case RESMSG_REGISTER:
        case RESMSG_UPDATE:
            free(msg->record.klass);
            break;

        case RESMSG_AUDIO:
            prop  = &msg->audio.property;
            match = &prop->match;
            free(msg->audio.group);
            free(prop->name);
            free(match->pattern);
            break;

        case RESMSG_VIDEO:
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


/* 
 * Local Variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim:set expandtab shiftwidth=4:
 */
