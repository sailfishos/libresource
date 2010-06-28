#include <stdlib.h>
#include <string.h>


#include "res-conn-private.h"
#include "res-set-private.h"


resset_t *resset_create(resconn_t     *rcon,
                        const char    *peer,
                        uint32_t       id,
                        resset_state_t state,
                        const char    *klass,
                        uint32_t       mode,
                        uint32_t       all,
                        uint32_t       opt,
                        uint32_t       share,
                        uint32_t       mask)
{
    resset_t *rset;

    if ((rset = malloc(sizeof(resset_t))) != NULL) {
    
        memset(rset, 0, sizeof(resset_t));
        rset->next        = rcon->any.rsets;
        rset->refcnt      = 1;
        rset->resconn     = rcon;
        rset->peer        = strdup(peer);
        rset->id          = id;
        rset->state       = state;
        rset->klass       = strdup(klass);
        rset->mode        = mode,
        rset->flags.all   = all;
        rset->flags.opt   = opt;
        rset->flags.share = share;
        rset->flags.mask  = mask;

        rcon->any.rsets  = rset;
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
    resconn_dbus_t  *rcon = &rset->resconn->dbus;
    resset_t        *prev;

    if (rset != NULL && --rset->refcnt <= 0) {
 
        for (prev = (resset_t *)&rcon->rsets;  prev->next;  prev = prev->next){
            if (prev->next == rset) {
                prev->next = rset->next;
                
                free(rset->peer);
                free(rset->klass);
                free(rset);
                
                break;
            }
        }
    }
}

void resset_update_flags(resset_t *rset,
                         uint32_t  all,
                         uint32_t  opt,
                         uint32_t  share,
                         uint32_t  mask)
{
    if (rset != NULL) {
        rset->flags.all   = all;
        rset->flags.opt   = opt;
        rset->flags.share = share;
        rset->flags.mask  = mask;
    }
}


resset_t *resset_find(resconn_t *rcon, const char *peer, uint32_t id)
{
    resset_t *rset;

    for (rset = rcon->any.rsets;   rset != NULL;   rset = rset->next) {
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
