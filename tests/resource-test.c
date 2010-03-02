#include <dbus/dbus.h>
#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <res-conn.h>

#include "resource.h"

static void advice_callback (resource_set_t *resource_set,
                             uint32_t        resources,
                             void           *userdata);
static void grant_callback (resource_set_t *resource_set,
                            uint32_t        resources,
                            void           *userdata);
static void simulate_server_response();

static void grant();
static void advice();
static void disconnect();

START_TEST (test_resource_set_create_and_destroy)
{
	resource_set_t *rs;

	// 1.1. should fail with invalid args
	fail_if(( rs = resource_set_create("player", RESOURCE_AUDIO_PLAYBACK, 0, 0, NULL          , 0)) != NULL );
	simulate_server_response();

	// 1.2. should succeed with valid args
	fail_if(( rs = resource_set_create("player", RESOURCE_AUDIO_PLAYBACK, 0, 0, grant_callback, 0)) == NULL );
	simulate_server_response();

	resource_set_destroy(rs);
	simulate_server_response();

}
END_TEST

START_TEST (test_resource_set_configure_resources)
{
	resource_set_t *rs;

	fail_if(( rs = resource_set_create("player", RESOURCE_AUDIO_PLAYBACK, 0, 0, grant_callback, 0)) == NULL );
	simulate_server_response();
	grant();
	resource_set_configure_resources(rs, RESOURCE_VIDEO_PLAYBACK, RESOURCE_AUDIO_PLAYBACK);
	simulate_server_response();
	resource_set_destroy(rs);
	simulate_server_response();
}
END_TEST

START_TEST (test_resource_set_configure_advice_callback)
{
	resource_set_t *rs;

	rs = resource_set_create("player", RESOURCE_AUDIO_PLAYBACK, 0, 0, grant_callback, 0);
	simulate_server_response();
	resource_set_configure_advice_callback(rs, advice_callback, NULL);
	simulate_server_response();
	resource_set_destroy(rs);
	simulate_server_response();
}
END_TEST

START_TEST (test_resource_set_acquire_and_release)
{
	resource_set_t *rs;

	rs = resource_set_create("player", RESOURCE_AUDIO_PLAYBACK, 0, 0, grant_callback, 0);
	simulate_server_response();
	advice();
	resource_set_acquire(rs);
	simulate_server_response();
	resource_set_release(rs);
	simulate_server_response();
	disconnect();
	resource_set_destroy(rs);
	simulate_server_response();
}
END_TEST

START_TEST (test_resource_set_configure_audio)
{
	resource_set_t *rs, *rs2;

	// 1.1. should return false when passed not an audio resource
	rs = resource_set_create("player", RESOURCE_VIDEO_PLAYBACK, 0, 0, grant_callback, 0);
	fail_if( resource_set_configure_audio(rs, "player", 0, NULL));
	simulate_server_response();
	resource_set_destroy(rs);
	simulate_server_response();

	// 2.1. create a set with a video playback resource
	rs = resource_set_create("player", RESOURCE_VIDEO_PLAYBACK | RESOURCE_AUDIO_PLAYBACK, 0, 0, grant_callback, 0);
	simulate_server_response();
	// 2.2. add a video resource
	fail_unless( resource_set_configure_resources(rs, RESOURCE_VIDEO_PLAYBACK, 0) );
	simulate_server_response();
	resource_set_destroy(rs);
	simulate_server_response();
	// 2.3. should succeed when passed an audio resource
	rs = resource_set_create("player", RESOURCE_VIDEO_PLAYBACK | RESOURCE_AUDIO_PLAYBACK, 0, 0, grant_callback, 0);
	simulate_server_response();
	fail_unless( resource_set_configure_audio(rs, "player", 0, NULL) );
	simulate_server_response();
	// 2.4. add another config
	fail_unless( resource_set_configure_audio(rs, "player", 0, NULL) );
	simulate_server_response();
	resource_set_destroy(rs);
	simulate_server_response();

}
END_TEST



TCase *
libresource_case (int desired_step_id)
{
	int step_id = 1;
	#define PREPARE_TEST(tc, fun) if (desired_step_id == 0 || step_id++ == desired_step_id)  tcase_add_test (tc, fun);

	TCase *tc_libresource = tcase_create ("libresource");
    tcase_set_timeout(tc_libresource, 60);
    PREPARE_TEST (tc_libresource, test_resource_set_create_and_destroy);
    PREPARE_TEST (tc_libresource, test_resource_set_configure_resources);
    PREPARE_TEST (tc_libresource, test_resource_set_configure_advice_callback);
    PREPARE_TEST (tc_libresource, test_resource_set_acquire_and_release);
    PREPARE_TEST (tc_libresource, test_resource_set_configure_audio);

    return tc_libresource;
}

int
main(int argc, char* argv[]) {
	Suite *s = suite_create ("libresource");

	int step_id = 0;
	if (argc == 2) {
		step_id = strtol(argv[1], NULL, 10);
	}
	suite_add_tcase (s, libresource_case(step_id));

	SRunner *sr = srunner_create (s);
	srunner_run_all (sr, CK_VERBOSE);
	int number_failed = srunner_ntests_failed (sr);
	srunner_free (sr);


	return number_failed;
}


static void grant_callback (resource_set_t *resource_set,
                            uint32_t        resources,
                            void           *userdata)
{
    char buf[512];

    (void)resource_set;
    (void)userdata;

    printf("*** %s(): granted resources %s\n", __FUNCTION__,
           resmsg_res_str (resources, buf, sizeof(buf)));
}

static void advice_callback (resource_set_t *resource_set,
                             uint32_t        resources,
                             void           *userdata)
{
    char buf[512];

    (void)resource_set;
    (void)userdata;

    printf("*** %s(): adviced resources %s\n", __FUNCTION__,
           resmsg_res_str (resources, buf, sizeof(buf)));
}


/* mocks */

////////////////////////////////////////////////////////////////
resconn_t *resourceConnection;
resset_t  *resSet;

resconn_t* resproto_init(resproto_role_t role, resproto_transport_t transport, ...)
{
    resconn_linkup_t callbackFunction;
    DBusConnection *dbusConnection;
    va_list args;

    va_start(args, transport);
    callbackFunction = va_arg(args, resconn_linkup_t);

    dbusConnection = va_arg(args, DBusConnection *);
    va_end(args);

    resourceConnection =(resconn_t *) calloc(1, sizeof(resconn_t));

    return resourceConnection;
}

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

static resproto_status_t status_cb_fun;
resmsg_t *last_message;

void simulate_server_response() {
	if (last_message) {
		last_message->status.errcod = 0;
		status_cb_fun(resSet, last_message);
		last_message = NULL;
	}
}

resset_t  *resconn_connect(resconn_t *connection, resmsg_t *message,
                           resproto_status_t callbackFunction)
{
    resSet = (resset_t *) calloc(1, sizeof(resset_t));
    resmsg_status_t *st;

    status_cb_fun = callbackFunction;
    last_message = message;
    st = &last_message->status;
    st->errcod = 0;
    st->errmsg = "resconn_connect stub msg";

    return resSet;
}

int resconn_disconnect(resset_t          *rset,
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


char *resmsg_res_str(uint32_t res, char *buf, int len)
{
    snprintf(buf, len, "0x%04x", res);

    return buf;
}

char *resmsg_type_str(resmsg_type_t type)
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

int resproto_send_message(resset_t          *rset,
                          resmsg_t          *resmsg,
                          resproto_status_t  status)
{
    resconn_t       *rcon = rset->resconn;
    resmsg_type_t    type = resmsg->type;
    int              success;

	resmsg->any.id = rset->id;
	last_message = resmsg;
	success = TRUE;

    return success;
}


DBusConnection *resource_get_dbus_bus(DBusBusType type, DBusError *err)
{
	return (DBusConnection *) 0xC0FFEE;
}

resproto_handler_t handlers[3];
int resproto_set_handler(union resconn_u *r, resmsg_type_t type, resproto_handler_t h)
{
    if (type == RESMSG_UNREGISTER) handlers[0] = h;
    if (type == RESMSG_GRANT)      handlers[1] = h;
    if (type == RESMSG_ADVICE)     handlers[2] = h;

    return 1;
}

static void grant()
{
    static resmsg_t msg  = {0};
    static resset_t rset = {0};
    if (handlers[1])  handlers[1](&msg, &rset, NULL);
}

static void advice()
{
    static resmsg_t msg  = {0};
    static resset_t rset = {0};
    if (handlers[2])  handlers[2](&msg, &rset, NULL);
}

static void disconnect()
{
    static resmsg_t msg  = {0};
    static resset_t rset = {0};
    if (handlers[0])  handlers[0](&msg, &rset, NULL);
}
