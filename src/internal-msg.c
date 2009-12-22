#include <stdlib.h>
#include <string.h>

#include "res-msg.h"
#include "internal-msg.h"

resmsg_t *resmsg_internal_copy_message(resmsg_t *src)
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

void resmsg_internal_destroy_message(resmsg_t *msg)
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


/* 
 * Local Variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim:set expandtab shiftwidth=4:
 */
