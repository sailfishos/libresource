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
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "res-conn-private.h"
#include "res-set-private.h"
#include "dbus-proto.h"
#include "internal-proto.h"
#include "visibility.h"

static resconn_t     *resconn_list;

#define VALID   1
#define INVALID 0

static int manager_valid_message[RESMSG_MAX] = {
    [RESMSG_REGISTER  ] = VALID   ,
    [RESMSG_UNREGISTER] = VALID   ,
    [RESMSG_UPDATE    ] = VALID   ,
    [RESMSG_ACQUIRE   ] = VALID   ,
    [RESMSG_RELEASE   ] = VALID   ,
    [RESMSG_GRANT     ] = INVALID ,
    [RESMSG_ADVICE    ] = INVALID ,
    [RESMSG_AUDIO     ] = VALID   ,
    [RESMSG_VIDEO     ] = VALID   ,
};
static int client_valid_message[RESMSG_MAX] = {
    [RESMSG_REGISTER  ] = INVALID ,
    [RESMSG_UNREGISTER] = VALID   ,
    [RESMSG_UPDATE    ] = INVALID ,
    [RESMSG_ACQUIRE   ] = INVALID ,
    [RESMSG_RELEASE   ] = VALID   ,
    [RESMSG_GRANT     ] = VALID   ,
    [RESMSG_ADVICE    ] = VALID   ,
    [RESMSG_AUDIO     ] = INVALID ,
    [RESMSG_VIDEO     ] = INVALID ,
};

#undef INVALID
#undef VALID


static int manager_link_handler(resconn_t *, char *, resproto_linkst_t);
static int client_link_handler(resconn_t *, char *, resproto_linkst_t);

static void resconn_list_add(resconn_t *);


resconn_t *resconn_init(resproto_role_t       role,
                        resproto_transport_t  transp,
                        va_list               args)
{
    static uint32_t  id;

    resconn_t *rcon;
    int        success;

    if ((rcon = malloc(sizeof(resconn_t))) != NULL) {

        memset(rcon, 0, sizeof(resconn_t));
        rcon->any.id      = ++id;
        rcon->any.role    = role;
        rcon->any.transp  = transp;

        switch (role) {

        case RESPROTO_ROLE_MANAGER:
            rcon->any.link    = manager_link_handler;
            rcon->any.valid   = manager_valid_message;

            switch (transp) {
            case RESPROTO_TRANSPORT_DBUS:
                success = resproto_dbus_manager_init(&rcon->dbus, args);
                break;
            case RESPROTO_TRANSPORT_INTERNAL:
                success = resproto_internal_manager_init(&rcon->internal,args);
                break;
            default:
                success = FALSE;
            }

            break;
            
        case RESPROTO_ROLE_CLIENT:
            rcon->any.link    = client_link_handler;
            rcon->any.valid   = client_valid_message;

            switch (transp) {
            case RESPROTO_TRANSPORT_DBUS:
                success = resproto_dbus_client_init(&rcon->dbus, args);
                break;
            case RESPROTO_TRANSPORT_INTERNAL:
                success = resproto_internal_client_init(&rcon->internal, args);
                break;
            default:
                success = FALSE;
            }

            break;
            
        default:
            success = FALSE;
            break;
        }

        if (!success)
            goto failed;

        resconn_list_add(rcon);
    }

    return rcon;

 failed:
    free(rcon);
    return NULL;
}


EXPORT resset_t *resconn_connect(resconn_t         *rcon,
                                 resmsg_t          *resmsg,
                                 resproto_status_t  status)
{
    resset_t *rset;

    if (rcon == NULL || rcon->any.killed ||
        rcon->any.role != RESPROTO_ROLE_CLIENT ||
        resmsg->type != RESMSG_REGISTER)
    {
        rset =  NULL;
    }
    else {
        rset = rcon->any.connect(rcon, resmsg);
        rcon->any.send(rset, resmsg, status);
    }

    return rset;
}

EXPORT int resconn_disconnect(resset_t          *rset,
                              resmsg_t          *resmsg,
                              resproto_status_t  status)
{
    resconn_t  *rcon = rset->resconn;
    int         success;

    if (rset         == NULL                          ||
        rset->state  != RESPROTO_RSET_STATE_CONNECTED ||
        resmsg->type != RESMSG_UNREGISTER               )
    {
        success = FALSE;
    }
    else {
        if ((success = rcon->any.send(rset, resmsg, status)))
            rcon->any.disconn(rset);
    }

    return success;
}


resconn_reply_t *resconn_reply_create(resmsg_type_t       type,
                                      uint32_t            serial,
                                      uint32_t            reqno,
                                      resset_t           *rset,
                                      resproto_status_t   status)
{
    resconn_any_t   *rcon = &rset->resconn->any;
    resconn_reply_t *reply;
    resconn_reply_t *last;

    for (last = (void *)&rcon->replies;   last->next;   last = last->next)
        ;

    if ((reply = malloc(sizeof(resconn_reply_t))) != NULL) {
        memset(reply, 0, sizeof(resconn_reply_t));
        reply->type     = type;
        reply->serial   = serial;
        reply->reqno    = reqno;
        reply->callback = status;
        reply->rset     = rset;
        resset_ref(rset);
                
        last->next = reply;
    }

    return reply;
}


void resconn_reply_destroy(void *ptr)
{
    resconn_reply_t *reply = (resconn_reply_t *)ptr;
    resset_t        *rset;
    resconn_t       *rcon;
    resconn_reply_t *prev;

    if (reply != NULL) {
        rset = reply->rset;
        if (rset != NULL && (rcon = rset->resconn) != NULL) {
            for (prev = (void *)&rcon->any.replies;
                 prev->next != NULL;
                 prev = prev->next)
            {
                if (prev->next == reply) {
                    prev->next = reply->next;
                    free(reply);
                    break;
                }
            }
        }
        resset_unref(rset);
    }    
}

resconn_reply_t *resconn_reply_find(resconn_t *rcon, uint32_t serial)
{
    resconn_reply_t *reply;

    for (reply = rcon->any.replies;   reply != NULL;   reply = reply->next) {
        if (serial == reply->serial)
            break;
    }

    return reply;
}


static int manager_link_handler(resconn_t         *rcon,
                                char              *peer,
                                resproto_linkst_t  state)
{
    resset_t            *rset;
    resset_t            *next;
    resmsg_t             resmsg;
    int                  found;
    resproto_handler_t   handler;


    (void)state;                /* supposed to be always RESPROTO_LINK_DOWN */

    found   = FALSE;
    handler = rcon->any.handler[RESMSG_UNREGISTER];

    memset(&resmsg, 0, sizeof(resmsg));
    resmsg.possess.type = RESMSG_UNREGISTER;

    for (rset = rcon->any.rsets;    rset != NULL;    rset = next) {
        next = rset->next;

        if (!strcmp(peer, rset->peer)) {
            if (handler && rset->state == RESPROTO_RSET_STATE_CONNECTED) {
                resmsg.possess.id = rset->id;
                handler(&resmsg, rset, NULL);
            }

            rcon->any.disconn(rset);

            found = TRUE;
        }
    }

    return found;
}

static int client_link_handler(resconn_t         *rcon,
                               char              *peer,
                               resproto_linkst_t  state)
{
    resset_t          *rset, *next;
    resmsg_t           resmsg;
    resproto_handler_t handler;

    switch (state) {

    case RESPROTO_LINK_UP:
        if (rcon->any.mgrup)
            rcon->any.mgrup(rcon);
        break;

    case RESPROTO_LINK_DOWN:
        handler = rcon->any.handler[RESMSG_UNREGISTER];

        memset(&resmsg, 0, sizeof(resmsg));
        resmsg.possess.type = RESMSG_UNREGISTER;

        for (rset = rcon->any.rsets;   rset != NULL;   rset = next) {
            next = rset->next;

            if (handler && rset->state == RESPROTO_RSET_STATE_CONNECTED) {
                resmsg.possess.id = rset->id;
                handler(&resmsg, rset, NULL);
            }

            rcon->any.disconn(rset);
        }
        break;

    default:
        break;
    }

    return FALSE;
}


static void resconn_list_add(resconn_t *rcon)
{
    if (rcon != NULL) {
        rcon->any.next  = resconn_list;
        resconn_list = rcon;
    }
}

resconn_t *resconn_list_iterate(resconn_t *rcon)
{
    if (rcon == NULL)
        rcon = (resconn_t *)&resconn_list;

    return rcon->any.next;
}

/* 
 * Local Variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim:set expandtab shiftwidth=4:
 */
