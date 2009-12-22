#include <stdlib.h>
#include <string.h>

#include "resproto.h"



resset_t *resset_create(resproto_t    *rp,
                        const char    *peer,
                        uint32_t       id,
                        resset_state_t state,
                        uint32_t       all,
                        uint32_t       share,
                        uint32_t       opt)
{
    resset_t *rset;

    if ((rset = malloc(sizeof(resset_t))) != NULL) {
    
        memset(rset, 0, sizeof(resset_t));
        rset->next        = rp->any.rsets;
        rset->refcnt      = 1;
        rset->resproto    = rp;
        rset->peer        = strdup(peer);
        rset->id          = id;
        rset->state       = state;
        rset->flags.all   = all;
        rset->flags.share = share;
        rset->flags.opt   = opt;

        rp->any.rsets  = rset;
    }

    return rset;
}

void resset_destroy(resset_t *rset)
{
    if (rset && rset->state != RESPROTO_RSET_STATE_KILLED) {
        rset->state = RESPROTO_RSET_STATE_KILLED;
        resset_unref(rset);
    }
}

void resset_ref(resset_t *rset)
{
    if (rset != NULL)
        rset->refcnt++;
}
 
void resset_unref(resset_t *rset)
{
    resproto_dbus_t  *rp = &rset->resproto->dbus;
    resset_t         *prev;
    resproto_reply_t *reply;
    resproto_reply_t *next;
    resmsg_t          resmsg;

    if (rset != NULL && --rset->refcnt <= 0) {
 
       for (prev = (resset_t *)&rp->rsets;
            prev->next != NULL;
            prev = prev->next)
        {
            if (prev->next == rset) {
                prev->next = rset->next;
                
                free(rset->peer);
                free(rset);
                
                break;
            }
        }
    }
}

resset_t *resset_find(resproto_t *rp, const char *peer, uint32_t id)
{
    resset_t *rset;

    for (rset = rp->any.rsets;   rset != NULL;   rset = rset->next) {
        if (!strcmp(peer, rset->peer) && id == rset->id)
            break;
    }

    return rset;
}



/* 
 * Local Variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim:set expandtab shiftwidth=4:
 */
