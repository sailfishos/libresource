#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <libgen.h>
#include <stdarg.h>
#include <errno.h>

#include <glib.h>
#include <dbus/dbus.h>

#include <res-conn.h>

#define REQHASH_BITS     6
#define REQHASH_DIM      (1 << REQHASH_BITS)
#define REQHASH_MASK     (REQHASH_DIM - 1)
#define REQHASH_INDEX(r) ((r) & REQHASH_MASK)


typedef struct {
    int             trace;
    uint32_t        id;
    char           *class;
    resmsg_rset_t   rset;
} conf_t;

typedef struct {
    uint32_t        value;
    const char     *name;
} rdef_t;

typedef struct {
    GIOChannel     *chan;
    guint           evsrc;
    int             accept;
    char            buf[256];
    int             len;        /* buffer length */
} input_t;

static void     install_signal_handlers(void);
static void     signal_handler(int, siginfo_t *, void *);

static void     create_mainloop(void);
static void     destroy_mainloop(void);
static void     run_mainloop(void);

static void     create_input(void);
static void     destroy_input(void);
static void     accept_input(int);
static void     print_input(void);
static void     parse_input(void);
static gboolean input_cb(GIOChannel *, GIOCondition, gpointer);

static void     create_dbus(void);
static void     destroy_dbus(void);

static void     create_manager(void);
static void     destroy_manager();
static void     connect_to_manager(resconn_t *rc);
static void     manager_status(resset_t *, resmsg_t *);
static void     manager_receive_message(resmsg_t *, resset_t *, void *);
static void     manager_send_message(resmsg_t *);

static void     print_error(char *, ...);
static void     print_message(char *fmt, ...);

static void     usage(int);
static void     parse_options(int, char **);
static char    *parse_class_string(char *);
static uint32_t parse_resource_list(char *, int);


static char            *exe_name = "";
static conf_t           config;
static GMainLoop       *main_loop;
static input_t          input;
static DBusConnection  *dconn;
static resconn_t       *rconn;
static resset_t        *rset;
static uint32_t         rid;
static uint32_t         reqno; 
static resmsg_type_t    reqtyp[REQHASH_DIM];

int main(int argc, char **argv)
{
    parse_options(argc, argv);
    install_signal_handlers();

    create_mainloop();
    create_input();
    create_dbus();
    create_manager();

    run_mainloop();

    print_message("exiting now ...");

    destroy_manager();
    destroy_dbus();
    destroy_input();
    destroy_mainloop();


  return 0;
}


static void install_signal_handlers(void)
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_sigaction = signal_handler;
    sa.sa_flags = SA_SIGINFO;

    if (sigaction(SIGHUP , &sa, NULL) < 0 ||
        sigaction(SIGTERM, &sa, NULL) < 0 ||
        sigaction(SIGINT , &sa, NULL) < 0   )
    {
        print_error("Failed to install signal handlers");
    }
}


static void signal_handler(int signo, siginfo_t *info, void *data)
{
#if 0
    ucontext_t *uc = (ucontext_t *)data;
#endif

    switch (signo) {
    case SIGHUP:
    case SIGTERM:
    case SIGINT:
        if (main_loop != NULL) {
            g_main_loop_quit(main_loop);
            break;
        }
        /* intentional fall over */

    default:
        exit(EINTR);
    }

    (void)info;
    (void)data;
}

static void create_mainloop(void)
{
    if ((main_loop = g_main_loop_new(NULL, FALSE)) == NULL) {
        print_error("Can't create G-MainLoop");
    }
}

static void destroy_mainloop(void)
{
    g_main_loop_unref(main_loop);
}

static void run_mainloop(void)
{
    g_main_loop_run(main_loop);
}

static void create_input(void)
{
    static GIOCondition cond = G_IO_IN | G_IO_HUP | G_IO_ERR;

    if (fcntl(0, F_SETFL, O_NONBLOCK) == -1)
        printf("Failed to set stdin to non-blocking mode");

    if ((input.chan = g_io_channel_unix_new(0)) == NULL)
        print_error("Failed to create input");

    input.evsrc = g_io_add_watch(input.chan, cond, input_cb, NULL);
}

static void destroy_input(void)
{
    g_source_remove(input.evsrc);
    g_io_channel_unref(input.chan);

    memset(&input, 0, sizeof(input_t));
}

static void accept_input(int accept)
{
    int current = input.accept;

    input.accept = accept;

    if (current != accept) {
        if (accept) {
            input.buf[0] = '\0';
            input.len = 0;
            
            print_message("accepting input");
            print_input();
        }
        else {
            if (input.accept) {
                print_message("rejecting input");
                fflush(stdout);
            }
        }
    }
}

static void print_input(void)
{
    printf(">%s", input.buf);
    fflush(stdout);
}

static char *skip_whitespaces(char *str)
{
    while (*str && (*str <= 0x20 || *str >= 0x7f))
        str++;

    return str;
}

static void parse_input(void)
{
    char     *str = input.buf;
    char     *p;
    char     *rs;
    uint32_t  res[3];
    int       i;
    resmsg_t  msg;


    if (!strncmp(str, "help", 4)) {
        str = skip_whitespaces(str + 4);
    }
    else if (!strncmp(str, "acquire", 7)) {
        str = skip_whitespaces(str + 7);

        memset(&msg, 0, sizeof(resmsg_t));
        msg.possess.type = RESMSG_ACQUIRE;
        manager_send_message(&msg);
    }
    else if (!strncmp(str, "release", 7)) {
        str = skip_whitespaces(str + 7);

        memset(&msg, 0, sizeof(resmsg_t));
        msg.possess.type = RESMSG_RELEASE;
        manager_send_message(&msg);
    }
    else if (!strncmp(str, "update", 6)) {
        rs = str = skip_whitespaces(str + 6);
  
        for (i = 0;  i < 3; i++) {
            if ((p = strchr(str, ':')) != NULL)
                *p = '\0';
            
            res[i] = *str ? parse_resource_list(str, 0) : 0;
            
            if (p)
                str = p + 1;
            else {
                while (*str)
                    str++;
            }
        }

        if (           !res[0]            ||
            ((res[0] | res[1]) != res[0]) ||
            ((res[0] | res[2]) != res[0])   )
        {
            print_message("invalid resource specification: '%s'", rs);
        }
        else {
            memset(&msg, 0, sizeof(resmsg_t));
            msg.record.type       = RESMSG_UPDATE;
            msg.record.rset.all   = res[0];
            msg.record.rset.share = res[1];
            msg.record.rset.opt   = res[2];
            msg.record.class      = config.class;
            manager_send_message(&msg);
        }
    }
    else {
        print_message("invalid input '%s'", input.buf);
        return;
    }
    
    if (*str) {
        print_message("garbage at the end of input: '%s'", str);
    }
}

static gboolean input_cb(GIOChannel *ch, GIOCondition cond, gpointer data)
{
    char  ignore[512];
    int   lincnt;
    int   rest;
    int   junk;
    char *p;

    (void)ch;
    (void)data;

    if (cond == G_IO_IN) {
        if (!input.accept) {
            while (read(0, ignore, sizeof(ignore)) > 0 || errno == EINTR)
                ;
        }
        else {
            while ((rest = sizeof(input.buf) - input.len) > 0) {

                if ((junk = read(0, input.buf + input.len, rest)) < 0) {
                    if (errno == EAGAIN)
                        break;
                    
                    if (errno == EINTR)
                        continue;
                    
                    print_error("error during input from terminal");
                }
                
                input.len += junk;
            }
            
            lincnt = 0;
            
            while ((p = strchr(input.buf, '\n')) != NULL) {
                *p++ = '\0';
                
                parse_input();
                
                if ((rest = input.len - (p - input.buf)) <= 0)
                    input.len = 0;
                else {
                    memmove(input.buf, p, rest);
                    input.len = rest;
                }
                
                input.buf[input.len] = '\0';
                
                lincnt++;
            }
            
            if (lincnt)
                print_input();
        }

        return TRUE;
    }

    return FALSE;
}


static void create_dbus(void)
{
    DBusError err;

    dbus_error_init(&err);

    if ((dconn = dbus_bus_get(DBUS_BUS_SESSION, &err)) == NULL) {
        print_error("Can't get session bus");
    }
}

static void destroy_dbus(void)
{
    dbus_connection_unref(dconn);

    dconn = NULL;
}

static void create_manager(void)
{
    int i;

    for (i = 0;  i < REQHASH_DIM; i++)
        reqtyp[i] = RESMSG_INVALID;

    rconn = resproto_init(RESPROTO_ROLE_CLIENT, RESPROTO_TRANSPORT_DBUS,
                          connect_to_manager, dconn);

    if (rconn == NULL)
        print_error("Can't initiate resprot");

    resproto_set_handler(rconn, RESMSG_UNREGISTER, manager_receive_message);
    resproto_set_handler(rconn, RESMSG_GRANT     , manager_receive_message);
    resproto_set_handler(rconn, RESMSG_ADVICE    , manager_receive_message);
    
    connect_to_manager(rconn);
}

static void destroy_manager(void)
{
}

static void connect_to_manager(resconn_t *rc)
{
    resmsg_t  resmsg;
    int       index;

    resmsg.record.type       = RESMSG_REGISTER;
    resmsg.record.id         = config.id;
    resmsg.record.reqno      = ++reqno;
    resmsg.record.rset.all   = config.rset.all;
    resmsg.record.rset.share = config.rset.share;
    resmsg.record.rset.opt   = config.rset.opt;
    resmsg.record.class      = config.class;

    rset = resconn_connect(rc, &resmsg, manager_status);

    index = REQHASH_INDEX(reqno);
    reqtyp[index] = RESMSG_REGISTER;
}

static void manager_status(resset_t *rset, resmsg_t *msg)
{
    char *str;
    int   idx;

    if (msg->type != RESMSG_STATUS)
        print_error("%s(): got confused with data structures", __FUNCTION__);
    else {
        idx = REQHASH_INDEX(msg->status.reqno);

        if (msg->status.errcod) {
            str = resmsg_type_str(reqtyp[idx]);
            print_message("%s request failed (%d): %s",
                          str, msg->status.errcod, msg->status.errmsg);
        }
        else {
            switch (reqtyp[idx]) {
            case RESMSG_REGISTER:        accept_input(TRUE);      break;
            case RESMSG_UNREGISTER:      accept_input(FALSE);     break;
            default:                                              break;
            }
        }

        reqtyp[idx] = RESMSG_INVALID;
    }
}

static void manager_receive_message(resmsg_t *msg, resset_t *rs, void *data)
{
    char  buf[80];

    (void)data;

    if (rs != rset)
        print_error("%s(): got confused with data structures", __FUNCTION__);
    else {
        switch (msg->type) {

        case RESMSG_UNREGISTER:
            print_message("unregister request from manager");
            accept_input(FALSE);
            break;

        case RESMSG_GRANT:
            print_message("manager granted the following resources: %s",
                          resmsg_res_str(msg->notify.resrc, buf, sizeof(buf)));
            break;

        case RESMSG_ADVICE:
            print_message("the following resources might be acquired: %s",
                          resmsg_res_str(msg->notify.resrc, buf, sizeof(buf)));
            break;

        default:
            print_message("%s(): unexpected message type %d (%s)",
                          __FUNCTION__, msg->type, resmsg_type_str(msg->type));
            print_input();
            break;
        }
    }
}

static void manager_send_message(resmsg_t *msg)
{
    int index;
    int success;

    msg->any.id    = config.id;
    msg->any.reqno = ++reqno;

    index = REQHASH_INDEX(msg->any.reqno);
    reqtyp[index] = msg->type;

    success = resproto_send_message(rset, msg, manager_status);

    if (!success) 
        print_message("failed to send %s request", resmsg_type_str(msg->type));
}



static void print_error(char *fmt, ...)
{
    va_list  ap;
    char     fmtbuf[512];

    snprintf(fmtbuf, sizeof(fmtbuf), "%s: %s\n", exe_name, fmt);

    va_start(ap, fmt);
    vprintf(fmtbuf, ap);
    va_end(ap);

    exit(errno);
}

static void print_message(char *fmt, ...)
{
    va_list  ap;
    char     fmtbuf[512];

    snprintf(fmtbuf, sizeof(fmtbuf), "%s: %s\n", exe_name, fmt);

    va_start(ap, fmt);
    vprintf(fmtbuf, ap);
    va_end(ap);
}

static void usage(int exit_code)
{
    printf("usage: %s [-h] [-t] "
           "[-o optional-resources] [-s shared-resources] "
           "class all-resources\n",
           exe_name);
    printf("\toptions:\n");
    printf("\t  h\tprint this help message and exit\n");
    printf("\t  t\ttrace messages\n");
    printf("\t  o\toptional resources. See 'resources' below for the "
           "syntax of\n\t\t<optional resources>\n");
    printf("\t  s\tshared resources. See 'resources' below for the "
           "syntax of\n\t\t<shared resources>\n");
    printf("\tclass:\n");
    printf("\t\tcall\t- for native or 3rd party telephony\n");
    printf("\t\tringtone - for ringtones\n");
    printf("\t\talarm\t - for alarm clock\n");
    printf("\t\tmedia\t - for media playback/recording\n");
    printf("\t\tdefault\t - for nonidentified applications\n");
    printf("\tresources:\n");
    printf("\t  comma separated list of the following resources\n");
    printf("\t\tAudioPlayback\n");
    printf("\t\tVideoPlayback\n");
    printf("\t\tAudioRecording\n");
    printf("\t\tVideoRecording\n");
    printf("\t  no whitespace allowed in the resource list.\n");

    exit(exit_code);
}

static void parse_options(int argc, char **argv)
{
    int   option;

    exe_name = strdup(basename(argv[0]));

    config.trace = FALSE;
    config.id    = 1;

    while ((option = getopt(argc, argv, "hto:s:")) != -1) {

        switch (option) {
            
        case 'h':   usage(0);                                            break;
        case 't':   config.trace      = TRUE;                            break;
        case 'o':   config.rset.opt   = parse_resource_list(optarg, 1);  break;
        case 's':   config.rset.share = parse_resource_list(optarg, 1);  break;
        default:    usage(EINVAL);                                       break;
        
        }
    }

    if (optind != argc - 2)
        usage(EINVAL);
    else {
        config.class    = parse_class_string(argv[optind]);
        config.rset.all = parse_resource_list(argv[optind+1], 1);
    }

    if ((config.rset.all | config.rset.opt) != config.rset.all) {
        print_error("optonal resources are not subset of all resources");
    }

    if ((config.rset.all | config.rset.share) != config.rset.all) {
        print_error("shared resources are not subset of all resources ");
    }
}

static char *parse_class_string(char *str)
{
    if (strcmp(str, "call"    ) &&
        strcmp(str, "ringtone") &&
        strcmp(str, "alarm"   ) &&
        strcmp(str, "media"   ) &&
        strcmp(str, "default" )    )
   {
       print_error("invalid class '%s'", str);
   }

    return str;
}

uint32_t parse_resource_list(char *rlist_str, int exit_if_error)
{
    static rdef_t  rdef[] = {
        { RESMSG_AUDIO_PLAYBACK ,  "AudioPlayback"  },
        { RESMSG_VIDEO_PLAYBACK ,  "VideoPlayback"  },
        { RESMSG_AUDIO_RECORDING,  "AudioRecording" },
        { RESMSG_VIDEO_RECORDING,  "VideoRecording" },
        {           0           ,       NULL        }
    };

    uint32_t  rlist = 0;
    char      buf[256];
    rdef_t   *rd;
    char     *p;
    char     *e;

    if (rlist_str == NULL)
        print_message("missing resource list");
    else {
        strncpy(buf, rlist_str, sizeof(buf));
        buf[sizeof(buf)-1] = '\0';

        p = buf;

        do {
            if ((e = strchr(p, ',')) != NULL)
                *e++ = '\0';

            for (rd = rdef;  rd->name != NULL;  rd++) {
                if (!strcmp(p, rd->name))
                    break;
            }

            if (!rd->value) {
                print_message("invalid resource list '%s'", rlist_str);
                rlist = 0;
                break;
            }

            rlist |= rd->value;

        } while((p = e) != NULL);
    }

    if (!rlist && exit_if_error)
        exit(EINVAL);

    return rlist;
}

/*
 * Local Variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
