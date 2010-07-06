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
#include <errno.h>

#include <res-conn.h>

#include "resource.h"
#include "resource-glue.h"
#include "visibility.h"

#include <res-conn.h>


typedef enum {
    client_created = 0,
    client_connecting,
    client_ready
} client_state_t;

typedef void (*request_complete_t)(resource_set_t *, uint32_t, void *,
                                   int32_t, const char *);

typedef struct request_s {
    struct request_s        *next;
    resmsg_type_t            msgtyp;
    uint32_t                 reqno;
    int                      busy;
    struct {
        request_complete_t function;
        void              *data;
    }                        cb;
} request_t;


#define RESOURCE_CONFIG_COMMON     \
    union resource_config_u *next; \
    uint32_t                 mask

typedef struct {
    RESOURCE_CONFIG_COMMON;
} any_config_t;

typedef struct {
    RESOURCE_CONFIG_COMMON;
    char                    *group;      /* audio group */
    pid_t                    pid;        /* PID of the streaming component */
    char                   *stream;     /* pulseaudio stream name */
} audio_config_t;

typedef union resource_config_u {
    any_config_t             any;
    audio_config_t           audio;
} resource_config_t;

typedef struct {
    resource_callback_t      function;
    void                    *data;
} callback_t;

struct resource_set_s {
    struct resource_set_s   *next;
    DBusConnection          *dbus;       /* D-Bus connection */
    char                    *klass;      /* resource class */
    uint32_t                 id;         /* resource id */
    uint32_t                 mode;
    resconn_t               *resconn;
    struct {
        uint32_t all;
        uint32_t opt;
    }                        resources;  /* libresource resources */
    client_state_t           client;     /* resource client state */
    int                      acquire;
    callback_t               grantcb;
    callback_t               advicecb;
    resource_config_t       *configs;
    resset_t                *resset;
    request_t               *reqlist;
};

static resource_set_t *rslist;
static uint32_t        rsid;
static uint32_t        reqno;

static DBusConnection *get_dbus(void);
static resconn_t      *get_manager(DBusConnection *);
static void            manager_is_up(resconn_t *);
static void            connect_to_manager(resconn_t *, resource_set_t *);
static void            disconnect_from_manager(resmsg_t *, resset_t *,void *);
static void            receive_grant_message(resmsg_t *, resset_t *, void *);
static void            receive_advice_message(resmsg_t *, resset_t *, void *);
static int             send_register_message(resource_set_t *, uint32_t);
static int             send_unregister_message(resource_set_t *, uint32_t);
static int             send_update_message(resource_set_t *, uint32_t);
static int             send_audio_message(resource_set_t *, uint32_t);
static int             send_acquire_message(resource_set_t *, uint32_t);
static int             send_release_message(resource_set_t *, uint32_t);
static void            connect_complete_cb(resource_set_t *, uint32_t, void *,
                                           int32_t, const char *);
static void            disconnect_complete_cb(resource_set_t *, uint32_t,
                                              void *, int32_t, const char *);
static void            request_complete_cb(resource_set_t *, uint32_t,
                                           void *, int32_t, const char *);
static void            status_cb(resset_t *, resmsg_t *);
static void            send_request(resource_set_t *);
static void            config_destroy(resource_config_t *);
static int             audio_config_create(resource_set_t *, const char *,
                                           pid_t pid, const char *);
static int             audio_config_update(resource_config_t *, const char *,
                                           pid_t, const char *);
static uint32_t        push_request(resource_set_t *, resmsg_type_t,
                                    request_complete_t, void *);
static request_t      *peek_request(resource_set_t *);
static request_t      *pop_request(resource_set_t *, uint32_t);
static void            destroy_request(request_t *);
static void            resource_log(const char *, ...);


EXPORT resource_set_t *resource_set_create(const char          *klass,
                                           uint32_t             mandatory,
                                           uint32_t             optional,
                                           uint32_t             mode,
                                           resource_callback_t  grantcb,
                                           void                *grantdata)
{
    DBusConnection *dbus    = get_dbus();
    resconn_t      *resconn = get_manager(dbus);
    resource_set_t *rs      = NULL;
    char            mbuf[256];
    char            obuf[256];

    if (klass && (mandatory || optional) && grantcb) {

        optional = optional & ~mandatory;

        if ((rs = malloc(sizeof(resource_set_t))) != NULL) {
            
            memset(rs, 0, sizeof(resource_set_t));
            rs->next    = rslist;
            rs->dbus    = dbus;
            rs->klass   = strdup(klass);
            rs->id      = rsid++;
            rs->mode    = mode;
            rs->resconn = resconn;
            rs->client  = client_created;
            rs->resources.all    = mandatory | optional;
            rs->resources.opt    = optional;
            rs->grantcb.function = grantcb;
            rs->grantcb.data     = grantdata;
            
            rslist = rs;

            resource_log("created resource set %u (klass '%s', "
                         "mandatory %s, optional %s)", rs->id, rs->klass,
                         resmsg_res_str(mandatory, mbuf, sizeof(mbuf)),
                         resmsg_res_str(optional , obuf, sizeof(obuf)));
            
            connect_to_manager(resconn, rs);
        }
    }

    return rs;
}

EXPORT void resource_set_destroy(resource_set_t *rs)
{
    push_request(rs, RESMSG_UNREGISTER, disconnect_complete_cb, NULL);
}


EXPORT int resource_set_configure_advice_callback(resource_set_t      *rs,
                                                  resource_callback_t  advcb,
                                                  void                *advdata)
{
    if (rs != NULL) {
        rs->advicecb.function = advcb;
        rs->advicecb.data = advdata;
    }
}


EXPORT int resource_set_configure_resources(resource_set_t *rs,
                                            uint32_t        mandatory,
                                            uint32_t        optional)
{
    int success = FALSE;
    uint32_t all;
    uint32_t rn;
    char mbuf[256];
    char obuf[256];

    if (rs != NULL) {

        optional = optional & ~mandatory;
        all = mandatory | optional;

        resource_log("updating resource set %u (klass '%s', "
                     "mandatory %s, optional %s)", rs->id, rs->klass,
                     resmsg_res_str(mandatory, mbuf, sizeof(mbuf)),
                     resmsg_res_str(optional , obuf, sizeof(obuf)));

        if (rs->resources.all != all || rs->resources.opt != optional) {

            rs->resources.all = mandatory | optional;
            rs->resources.opt = optional;
    
            rn = push_request(rs, RESMSG_UPDATE, NULL,NULL);

            success = rn ? TRUE : FALSE;
        }
    }

    return success;
}


EXPORT int resource_set_configure_audio(resource_set_t *rs,
                                        const char     *group,
                                        pid_t           pid,
                                        const char     *stream)
{
    resource_config_t *prev;
    resource_config_t *cfg;
    int                need_update;

    if (!(rs->resources.all & RESOURCE_AUDIO_PLAYBACK))
        return FALSE;

    for (prev = (resource_config_t *)&rs->configs; 
         (cfg = prev->any.next) != NULL;
         prev = prev->any.next)
    {
        if (cfg->any.mask == RESOURCE_AUDIO_PLAYBACK) {
            need_update = audio_config_update(cfg, group, pid,stream);
            break;
        }
    }

    if (cfg == NULL) 
        need_update = audio_config_create(rs, group, pid,stream);

    if (need_update)
        push_request(rs, RESMSG_AUDIO, NULL,NULL);

    return TRUE;
}

EXPORT int resource_set_acquire(resource_set_t *rs)
{
    if (rs && !rs->acquire) {
        rs->acquire = TRUE;
        push_request(rs, RESMSG_ACQUIRE, NULL,NULL);
    }

    return TRUE;
}

EXPORT int resource_set_release(resource_set_t *rs)
{
    if (rs && rs->acquire) {
        rs->acquire = FALSE;
        push_request(rs, RESMSG_RELEASE, NULL,NULL);
    }

    return TRUE;
}



static DBusConnection *get_dbus(void)
{
    static DBusConnection *dbus = NULL;

    DBusError err;

    if (dbus == NULL) {
        dbus_error_init(&err);

        dbus = resource_get_dbus_bus(DBUS_BUS_SESSION, &err);

        if (dbus_error_is_set(&err)) {
            /* TODO: some more distinctive errno setting would not harm :) */
            errno = EIO;
            dbus_error_free(&err);
        }
    }

    return dbus;
}

static resconn_t *get_manager(DBusConnection *dbus)
{
    static resconn_t  *mgr = NULL;

    if (mgr == NULL && dbus != NULL) {
        mgr = resproto_init(RESPROTO_ROLE_CLIENT, RESPROTO_TRANSPORT_DBUS,
                            manager_is_up, dbus);

        resproto_set_handler(mgr, RESMSG_UNREGISTER, disconnect_from_manager);
        resproto_set_handler(mgr, RESMSG_GRANT     , receive_grant_message  );
        resproto_set_handler(mgr, RESMSG_ADVICE    , receive_advice_message );
    }

    return mgr;
}

static void manager_is_up(resconn_t *resconn)
{
    resource_set_t *rs;

    for (rs = rslist;  rs != NULL;  rs = rs->next) {
        if (rs->resconn == resconn && rs->client == client_created)
            connect_to_manager(resconn, rs);
    }
}

static void connect_to_manager(resconn_t *resconn, resource_set_t *rs)
{
    resource_config_t *cfg;
 
    if (rs != NULL) {
        rs->client = client_connecting;

        push_request(rs, RESMSG_REGISTER, connect_complete_cb, NULL);

        for (cfg = rs->configs;  cfg != NULL;  cfg = cfg->any.next) {
            switch (cfg->any.mask) {
            case RESMSG_AUDIO_PLAYBACK:
                push_request(rs, RESMSG_AUDIO, NULL,NULL);
            default:
                break;
            }
        }

        if (rs->acquire)
            push_request(rs, RESMSG_ACQUIRE, NULL,NULL);
    }
}

static void disconnect_from_manager(resmsg_t *msg, resset_t *resset,void *data)
{
    resource_set_t *rs = resset->userdata;

    (void)data;

    if (rs != NULL && resset == rs->resset) {
        rs->client = client_created;
    }
}


static void receive_grant_message(resmsg_t *msg, resset_t *resset, void *data)
{
    resource_set_t *rs = resset->userdata;
    uint32_t        rn = msg->notify.reqno;
    uint32_t        gr = msg->notify.resrc;

    (void)data;

    if (rs != NULL && resset == rs->resset) {
        resource_log("recived grant %u (resources 0x%x)", rn, gr);

        rs->grantcb.function(rs, gr, rs->grantcb.data);
    }
}

static void receive_advice_message(resmsg_t *msg, resset_t *resset, void *data)
{
    resource_set_t *rs  = resset->userdata;
    uint32_t        adv = msg->notify.resrc;

    (void)data;

    if (rs != NULL && resset == rs->resset) {
        if (rs->advicecb.function) {
            rs->advicecb.function(rs, adv, rs->advicecb.data);
        }
    }
}

static int send_register_message(resource_set_t *rs, uint32_t rn)
{
    resset_t *resset;
    resmsg_t  msg;

    if (rs->resconn != NULL) {
        resource_log("sending register message");

        memset(&msg, 0, sizeof(msg));
        msg.record.type     = RESMSG_REGISTER;
        msg.record.id       = rs->id;
        msg.record.reqno    = rn;
        msg.record.rset.all = rs->resources.all;
        msg.record.rset.opt = rs->resources.opt;
        msg.record.klass    = rs->klass;
        msg.record.mode     = rs->mode;

        resset = resconn_connect(rs->resconn, &msg, status_cb);

        if (resset != NULL) {
            rs->resset = resset;
            resset->userdata = rs;
        }
    }

    return rs->resset ? TRUE : FALSE;
}

static int send_unregister_message(resource_set_t *rs, uint32_t rn)
{
    resset_t *resset  = rs->resset;
    int       success = FALSE;
    resmsg_t  msg;

    if (resset != NULL && (void *)rs == resset->userdata) {
        resource_log("sending unregister message");

        memset(&msg, 0, sizeof(msg));
        msg.possess.type     = RESMSG_UNREGISTER;
        msg.possess.id       = rs->id;
        msg.possess.reqno    = rn;

        success = resconn_disconnect(resset, &msg, status_cb);
    }

    return success;
}

static int send_update_message(resource_set_t *rs, uint32_t rn)
{
    resset_t *resset  = rs->resset;
    int       success = FALSE;
    resmsg_t  msg;

    if (resset != NULL && (void *)rs == resset->userdata) {
        resource_log("sending update message");

        memset(&msg, 0, sizeof(msg));
        msg.record.type     = RESMSG_UPDATE;
        msg.record.id       = rs->id;
        msg.record.reqno    = rn;
        msg.record.rset.all = rs->resources.all;
        msg.record.rset.opt = rs->resources.opt;
        msg.record.klass    = rs->klass;
        msg.record.mode     = rs->mode;

        success = resproto_send_message(resset, &msg, status_cb);
    }

    return success;
}

static int send_audio_message(resource_set_t *rs, uint32_t rn)
{
    resset_t          *resset = rs->resset;
    int                success = FALSE;
    resource_config_t *cfg;
    resmsg_t           msg;
    char              *stream;

    if (resset != NULL && (void *)rs == resset->userdata) {
        for (cfg = rs->configs;  cfg != NULL;   cfg = cfg->any.next) {
            if (cfg->any.mask == RESOURCE_AUDIO_PLAYBACK) {

                stream = cfg->audio.stream ? cfg->audio.stream : (char *)"";

                resource_log("sending audio message");

                memset(&msg, 0, sizeof(msg));
                msg.audio.type  = RESMSG_AUDIO;
                msg.audio.id    = rs->id;
                msg.audio.reqno = rn;
                msg.audio.group = cfg->audio.group;
                msg.audio.pid   = cfg->audio.pid;
                msg.audio.property.name = (char*)"media.name";
                msg.audio.property.match.method  = resmsg_method_equals;
                msg.audio.property.match.pattern = stream;
        
                success = resproto_send_message(resset, &msg, status_cb);

                break;
            }
        } /* for */
    }

    return success;
}


static int send_acquire_message(resource_set_t *rs, uint32_t rn)
{
    resset_t *resset  = rs->resset;
    int       success = FALSE;
    resmsg_t  msg;

    if (resset != NULL && (void *)rs == resset->userdata) {
        resource_log("sending acquire message");

        memset(&msg, 0, sizeof(msg));
        msg.possess.type  = RESMSG_ACQUIRE;
        msg.possess.id    = rs->id;
        msg.possess.reqno = rn;
        
        success = resproto_send_message(resset, &msg, status_cb);
    }

    return success;
}

static int send_release_message(resource_set_t *rs, uint32_t rn)
{
    resset_t *resset  = rs->resset;
    int       success = FALSE;
    resmsg_t  msg;

    if (resset != NULL && (void *)rs == resset->userdata) {
        resource_log("sending release message");

        memset(&msg, 0, sizeof(msg));
        msg.possess.type  = RESMSG_RELEASE;
        msg.possess.id    = rs->id;
        msg.possess.reqno = rn;
        
        success = resproto_send_message(resset, &msg, status_cb);
    }

    return success;
}

static void connect_complete_cb(resource_set_t *rs, uint32_t rn, void *data,
                                int32_t errcod, const char *errmsg)
{
    if (errcod != 0) {
        resource_log("failed to connect resource manager: %d %s",
                     errcod, errmsg);
        rs->client = client_created;
    }
    else {
        resource_log("resource set %u is ready", rs->id);
        rs->client = client_ready;
    }
}

static void disconnect_complete_cb(resource_set_t *rs, uint32_t no, void *data,
                                   int32_t errcod, const char *errmsg)
{
    resource_set_t    *prev;
    resource_config_t *cfg;
    resource_config_t *cn;
    request_t         *rq;
    request_t         *rn;

    (void)no;
    (void)data;
    (void)errcod;
    (void)errmsg;

    for (prev = (resource_set_t *)&rslist;   prev->next;   prev = prev->next) {
        if (rs == prev->next) {
            resource_log("resource set %u is going to be destroyed", rs->id);

            prev->next = rs->next;

            free(rs->klass);

            for (cfg = rs->configs;  cfg;   cfg = cn) {
                cn = cfg->any.next;
                config_destroy(cfg);
            }
    
            for (rq = rs->reqlist;  rq;  rq = rn) {
                rn = rq->next;
                destroy_request(rq);
            }

            free(rs);

            return;
        }
    }
}

static void request_complete_cb(resource_set_t *rs, uint32_t rn, void *data,
                                int32_t errcod, const char *errmsg)
{
    resource_log("request %u completed. status %d %s", rn, errcod, errmsg);

    if (errcod != 0) {
        rs->grantcb.function(rs, 0, rs->grantcb.data);
    }
}

static void status_cb(resset_t *resset, resmsg_t *msg)
{
    resource_set_t  *rs = resset->userdata;
    resmsg_status_t *st = &msg->status;
    resmsg_type_t    mt;
    request_t       *rq;

    if (rs != NULL && rs->resset == resset) {

        resource_log("%s(%u) status: %d '%s'", __FUNCTION__, 
                     st->reqno, st->errcod, st->errmsg);

        if ((rq = pop_request(rs, msg->status.reqno)) != NULL) {

            mt = rq->msgtyp;

            if (rq->cb.function != NULL) {
                rq->cb.function(rs, msg->any.reqno, rq->cb.data,
                                st->errcod, st->errmsg);
            }

            destroy_request(rq);

            if (mt != RESMSG_UNREGISTER)
                send_request(rs);
        }
    }
}

static void send_request(resource_set_t *rs)
{
    request_t *rq;
    uint32_t   rn;
    int        success;

    while ((rq = peek_request(rs)) != NULL) {

        if (rq == NULL || rq->busy)
            break;

        if (rs->client == client_created)
            continue;

        rn = rq->reqno;

        switch (rq->msgtyp) {
        case RESMSG_REGISTER:   success = send_register_message(rs, rn); break;
        case RESMSG_UNREGISTER: success = send_unregister_message(rs,rn);break;
        case RESMSG_UPDATE:     success = send_update_message(rs, rn);   break;
        case RESMSG_AUDIO:      success = send_audio_message(rs, rn);    break;
        case RESMSG_ACQUIRE:    success = send_acquire_message(rs, rn);  break;
        case RESMSG_RELEASE:    success = send_release_message(rs, rn);  break;
        default:                success = FALSE;                         break;
        }

        if (success) {
            rq->busy = TRUE;
            break;
        }

        if (rq->msgtyp == RESMSG_REGISTER)
            rs->client = client_created;

        resource_log("failed to send %s message", resmsg_type_str(rq->msgtyp));

        if (pop_request(rs, rn) == rq)
            destroy_request(rq);

    } /* while */
}

static void config_destroy(resource_config_t *cfg)
{
    if (cfg->any.mask == RESOURCE_AUDIO_PLAYBACK) {
        free(cfg->audio.group);
        free(cfg->audio.stream);
    }

    free(cfg);
}

static int audio_config_create(resource_set_t *rs,
                               const char     *group,
                               pid_t           pid,
                               const char     *stream)
{
    resource_config_t *cfg;
    int need_update = FALSE;
    
    if (group || !pid || stream) {
        if ((cfg = malloc(sizeof(resource_config_t))) != NULL) {
            memset(cfg, 0, sizeof(resource_config_t));
            cfg->audio.next   = rs->configs;
            cfg->audio.mask   = RESOURCE_AUDIO_PLAYBACK;
            cfg->audio.group  = group ? strdup(group) : NULL;
            cfg->audio.pid    = pid;
            cfg->audio.stream = stream ? strdup(stream) : NULL;

            rs->configs = cfg;

            need_update = TRUE;
        }
    }

    return need_update;
}

static int audio_config_update(resource_config_t *cfg,
                               const char        *group,
                               pid_t              pid,
                               const char        *stream)
{
    char   *oldstr;
    pid_t   oldpid;
    int     need_update = FALSE;


    if (group) {
        oldstr = cfg->audio.group;

        if (!oldstr || (oldstr  && strcmp(oldstr, group))) {
            need_update = TRUE;
            free(oldstr);
            cfg->audio.group = strdup(group);
        }
    }

    if (pid) {
        oldpid = cfg->audio.pid;
        
        if (!oldpid || (oldpid && (oldpid != pid))) {
            need_update = TRUE;
            cfg->audio.pid = pid;
        }
    }

    if (stream) {
        oldstr = cfg->audio.stream;
        
        if (!oldstr || (oldstr  && strcmp(oldstr, stream))) {
            need_update = TRUE;
            free(oldstr);
            cfg->audio.stream = strdup(stream);
        }
    }

    return need_update;
}



static uint32_t push_request(resource_set_t     *rs,
                             resmsg_type_t       msgtyp,
                             request_complete_t  callback,
                             void               *data)
{
    request_t *last;
    request_t *rq;
    uint32_t   rn;

    if (rs->client == client_created) 
        rn = 0;
    else {

        for (last = (void*)&rs->reqlist;  last->next;  last = last->next)
            ;
    
        if ((rq = malloc(sizeof(request_t))) == NULL)
            rn = 0;
        else {
            rn = ++reqno;
            
            memset(rq, 0, sizeof(request_t));
            rq->msgtyp      = msgtyp;
            rq->reqno       = rn;
            rq->cb.function = callback;
            rq->cb.data     = data;
            
            last->next = rq;
            
            resource_log("pushed %u %s request", rq->reqno,
                         resmsg_type_str(msgtyp));
            
            send_request(rs);
        }
    }

    return rn;
}

static request_t *peek_request(resource_set_t *rs)
{
    return rs->reqlist;
}



static request_t *pop_request(resource_set_t *rs, uint32_t rn)
{
    request_t *prev;
    request_t *rq;

    for (prev = (void*)&rs->reqlist;  (rq = prev->next);  prev = prev->next) {
        if (rq->reqno == rn) {

            resource_log("popping message %u (%s message)",
                         rn, resmsg_type_str(rq->msgtyp));

            prev->next = rq->next;
            rq->next = NULL;

            break;
        }
    }

    return rq;
}

static void destroy_request(request_t *rq)
{
    free(rq);
}

static void resource_log(const char *fmt, ...)
{
    static int got_env;
    static int printit;

    va_list  ap;
    char    *envstr;
    char    *end;
    char     buf[1024];

    if (!got_env) {
        if ((envstr = getenv("LIBRESOURCE_DEBUG")) != NULL) {
            printit = strtol(envstr, &end, 10);

            if (printit < 0 || !envstr[0] || *end)
                printit = 0;

            got_env = TRUE;

            if (printit)
                printf("resource: logging turned on\n");
        }
    }

    if (printit) {
        snprintf(buf, sizeof(buf), "resource: %s\n", fmt);
        
        va_start(ap, fmt);
        
        vprintf(buf, ap);
        
        va_end(ap);
    }
}



/* 
 * Local Variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim:set expandtab shiftwidth=4:
 */
