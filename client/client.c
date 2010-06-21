#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <getopt.h>
#include <libgen.h>
#include <stdarg.h>
#include <errno.h>

#include <glib.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <res-conn.h>

#define REQHASH_BITS     6
#define REQHASH_DIM      (1 << REQHASH_BITS)
#define REQHASH_MASK     (REQHASH_DIM - 1)
#define REQHASH_INDEX(r) ((r) & REQHASH_MASK)


typedef struct {
    int             trace;
    uint32_t        id;
    char           *klass;
    uint32_t        mode;
    resmsg_rset_t   rset;
    int             verbose;
    DBusBusType     bustype;
    int             allow_unknown;
} conf_t;

typedef struct {
    uint32_t        value;
    const char     *name;
} rdef_t;

typedef struct {
    uint32_t        value;
    const char     *name;
} mdef_t;

typedef struct {
    GIOChannel     *chan;
    guint           evsrc;
    int             accept;
    char            buf[256];
    int             len;        /* buffer length */
} input_t;

static void         install_signal_handlers(void);
static void         signal_handler(int, siginfo_t *, void *);

static void         create_mainloop(void);
static void         destroy_mainloop(void);
static void         run_mainloop(void);
static void         break_out_mainloop(void);

static void         create_input(void);
static void         destroy_input(void);
static void         accept_input(int);
static void         print_input(void);
static void         parse_input(void);
static gboolean     input_cb(GIOChannel *, GIOCondition, gpointer);

static void         create_dbus(void);
static void         destroy_dbus(void);

static void         create_manager(void);
static void         destroy_manager();
static void         connect_to_manager(resconn_t *);
static void         disconnect_from_manager(void);
static void         manager_status(resset_t *, resmsg_t *);
static void         manager_receive_message(resmsg_t *, resset_t *, void *);
static void         manager_send_message(resmsg_t *);

static void         print_error(char *, ...);
static void         print_message(char *fmt, ...);

static void         usage(int);
static void         parse_options(int, char **);
static char        *parse_class_string(char *);
static uint32_t     parse_resource_list(char *, int);
static uint32_t     parse_mode_values(char *, int);
static DBusBusType  parse_bustype(char *, int);


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

static void break_out_mainloop(void)
{
    g_main_loop_quit(main_loop);
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
    if (input.accept) {
        printf(">%s", input.buf);
        fflush(stdout);
    }
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
    uint32_t  res[4];
    char     *audiogr;
    uint32_t  pid;
    char     *name;
    char     *value;
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
  
        for (i = 0;  i < 4; i++) {
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
            msg.record.rset.opt   = res[1];
            msg.record.rset.share = res[2];
            msg.record.rset.mask  = res[3];
            msg.record.klass      = config.klass;
            manager_send_message(&msg);
        }
    }
    else if (!strncmp(str, "audio", 5)) {
        p = skip_whitespaces(str + 5);

        do { 
            if (!isalpha(p[0])) {
                print_message("invalid audio group");
                break;
            }
            for (audiogr = p; *p;  p++) {
                if (*p == ' ' || *p == '\t') {
                    *p++ = '\0';
                    p = skip_whitespaces(p);
                    break;
                }
            }

            if (!(pid = strtoul(p, &p, 10)) || (*p != ' ' && *p != '\t')) {
                print_message("invalid pid");
                break;
            }

            name = skip_whitespaces(p);

            if ((p = strchr(name, ':')) == NULL) {
                print_message("invalid property specification: '%s'", name);
                break;
            }

            *p++ = '\0';
            value = p;
            str = strchrnul(value, ' ');
            *str = '\0';
            
            memset(&msg, 0, sizeof(resmsg_t));
            msg.audio.type  = RESMSG_AUDIO;
            msg.audio.group = audiogr;
            msg.audio.pid   = pid;
            msg.audio.property.name = name;
            msg.audio.property.match.method  = resmsg_method_equals;
            msg.audio.property.match.pattern = value; 
            
            manager_send_message(&msg);
        } while (0);
    } 
    else if (!strncmp(str, "disconnect", 10)) {
        disconnect_from_manager();
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
    const char *busname;
    DBusError   err;

    switch (config.bustype) {
    case DBUS_BUS_SYSTEM:    busname = "system";        break;
    case DBUS_BUS_SESSION:   busname = "session";       break;
    default:              /* should not come here */    exit(EINVAL);
    }


    dbus_error_init(&err);

    if ((dconn = dbus_bus_get(config.bustype, &err)) == NULL)
        print_error("Can't get %s bus", busname);

    dbus_connection_setup_with_g_main(dconn, NULL);

    print_message("using D-Bus %s bus", busname);
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
    resmsg.record.rset.opt   = config.rset.opt;
    resmsg.record.rset.share = config.rset.share;
    resmsg.record.rset.mask  = config.rset.mask;
    resmsg.record.klass      = config.klass;
    resmsg.record.mode       = config.mode;

    rset = resconn_connect(rc, &resmsg, manager_status);

    index = REQHASH_INDEX(reqno);
    reqtyp[index] = RESMSG_REGISTER;
}

static void disconnect_from_manager(void)
{
    resmsg_t  resmsg;
    int       index;

    resmsg.possess.type  = RESMSG_UNREGISTER;
    resmsg.possess.id    = config.id;
    resmsg.possess.reqno = ++reqno;

    resconn_disconnect(rset, &resmsg, manager_status);

    index = REQHASH_INDEX(reqno);
    reqtyp[index] = RESMSG_UNREGISTER;
}

static void manager_status(resset_t *rset, resmsg_t *msg)
{
    char          *str;
    int            idx;
    resmsg_type_t  type;

    if (msg->type != RESMSG_STATUS)
        print_error("%s(): got confused with data structures", __FUNCTION__);
    else {
        idx = REQHASH_INDEX(msg->status.reqno);
        type = reqtyp[idx];

        if (msg->status.errcod) {
            str = resmsg_type_str(reqtyp[idx]);
            print_message("%s request failed (%d): %s",
                          str, msg->status.errcod, msg->status.errmsg);
            print_input();
        }
        else {
            switch (type) {
            case RESMSG_REGISTER:
                accept_input(TRUE);
                break;
            case RESMSG_UNREGISTER:
                accept_input(FALSE);
                break_out_mainloop();
                break;
            default:
                break;
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
            resmsg_res_str(msg->notify.resrc, buf, sizeof(buf));
            if (config.verbose)
                print_message("manager granted the resources: %s", buf);
            else 
                print_message("granted: %s", buf);
            print_input();
            break;

        case RESMSG_ADVICE:
            resmsg_res_str(msg->notify.resrc, buf, sizeof(buf));
            if (config.verbose)
                print_message("resources might be acquired: %s", buf);
            else
                print_message("advice: %s", buf);
            print_input();
            break;

        default:
            if (config.verbose) {
                print_message("%s(): unexpected message type %d (%s)",
                              __FUNCTION__, msg->type, 
                              resmsg_type_str(msg->type));
            }
            else {
                print_message("error: wrong message (%d)", msg->type);
            }
            
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

    if (config.verbose) {
        snprintf(fmtbuf, sizeof(fmtbuf), "%s%s: %s\n",
                 input.accept ? "\n" : "", exe_name, fmt);
    }
    else {
        snprintf(fmtbuf, sizeof(fmtbuf), "%s%s\n",
                 input.accept ? "\n" : "", fmt);
    }

    va_start(ap, fmt);
    vprintf(fmtbuf, ap);
    va_end(ap);

    exit(errno);
}

static void print_message(char *fmt, ...)
{
    va_list  ap;
    char     fmtbuf[512];

    if (config.verbose) {
        snprintf(fmtbuf, sizeof(fmtbuf), "%s%s: %s\n",
                 input.accept ? "\n" : "", exe_name, fmt);
    }
    else {
        snprintf(fmtbuf, sizeof(fmtbuf), "%s%s\n",
                 input.accept ? "\n" : "", fmt);
    }

    va_start(ap, fmt);
    vprintf(fmtbuf, ap);
    va_end(ap);
}

static void usage(int exit_code)
{
    printf("usage: %s [-h] [-t] [-v] [-u] [-d bus-type] [-f mode-values]"
           "[-o optional-resources] [-s shared-resources -m shared-mask] "
           "class all-resources\n",
           exe_name);
    printf("\toptions:\n");
    printf("\t  h\tprint this help message and exit\n");
    printf("\t  t\ttrace messages\n");
    printf("\t  v\tverbose printouts\n");
    printf("\t  u\tallow 'unknown' (ie. other than listed below) classes");
    printf("\t  d\tbus-type. Either 'system' or 'session'\n");
    printf("\t  f\tmode values. See 'modes' below for the "
           "\n\t\tsyntax of <mode-values>\n");
    printf("\t  o\toptional resources. See 'resources' below for the "
           "syntax of\n\t\t<optional-resources>\n");
    printf("\t  s\tshared resources. See 'resources' below for the "
           "syntax of\n\t\t<shared-resources>\n");
    printf("\t  m\tshared resource mask. See 'resources' below for the "
           "syntax of\n\t\t<shared-mask>\n");
    printf("\tclass:\n");
    printf("\t\tcall\t  - for native or 3rd party telephony\n");
    printf("\t\tcamera\t  - for photo applications\n");
    printf("\t\tringtone  - for ringtones\n");
    printf("\t\talarm\t  - for alarm clock\n");
    printf("\t\tnavigator - for mapping applications\n");
    printf("\t\tgame\t  - for gaming\n");
    printf("\t\tplayer\t  - for media playback/recording\n");
    printf("\t\tevent\t  - for messaging and other event notifications\n");
    printf("\t\tbackground - for thumbnailing etc\n");
    printf("\tresources:\n");
    printf("\t  comma separated list of the following resources\n");
    printf("\t\tAudioPlayback\n");
    printf("\t\tVideoPlayback\n");
    printf("\t\tAudioRecording\n");
    printf("\t\tVideoRecording\n");
    printf("\t\tVibra\n");
    printf("\t\tLeds\n");
    printf("\t\tBackLight\n");
    printf("\t\tSystemButton\n");
    printf("\t\tLockButton\n");
    printf("\t\tScaleButton\n");
    printf("\t\tSnapButton\n");
    printf("\t\tLensCover\n");
    printf("\t  no whitespace allowed in the resource list.\n");
    printf("\tmodes:\n");
    printf("\t  comma separated lis of the following modes\n");
    printf("\t\tAutoRelease\n");

    exit(exit_code);
}

static void parse_options(int argc, char **argv)
{
    int   option;

    exe_name = strdup(basename(argv[0]));

    config.trace = FALSE;
    config.id    = 1;

    while ((option = getopt(argc, argv, "htvud:f:s:o:m:")) != -1) {

        switch (option) {
            
        case 'h': usage(0);                                              break;
        case 't': config.trace         = TRUE;                           break;
        case 'v': config.verbose       = TRUE;                           break;
        case 'd': config.bustype       = parse_bustype(optarg, 1);       break;
        case 'f': config.mode          = parse_mode_values(optarg, 1);   break;
        case 'o': config.rset.opt      = parse_resource_list(optarg, 1); break;
        case 's': config.rset.share    = parse_resource_list(optarg, 1); break;
        case 'm': config.rset.mask     = parse_resource_list(optarg, 1); break;
        case 'u': config.allow_unknown = TRUE;                           break;
        default:  usage(EINVAL);                                         break;
        
        }
    }

    if (optind != argc - 2)
        usage(EINVAL);
    else {
        config.klass    = parse_class_string(argv[optind]);
        config.rset.all = parse_resource_list(argv[optind+1], 1);
    }

    if ((config.rset.all | config.rset.opt) != config.rset.all) {
        print_error("optonal resources are not subset of all resources");
    }

    if ((config.rset.all | config.rset.mask) != config.rset.all) {
        print_error("shared resource mask is not subset of all resources ");
    }

    if ((config.rset.mask | config.rset.share) != config.rset.mask) {
        print_error("shared resource values are not subset of shared mask ");
    }
}

static char *parse_class_string(char *str)
{
    if (strcmp(str, "call"      ) &&
        strcmp(str, "camera"    ) &&
        strcmp(str, "ringtone"  ) &&
        strcmp(str, "alarm"     ) &&
        strcmp(str, "navigator" ) &&
        strcmp(str, "game"      ) &&
        strcmp(str, "player"    ) &&
        strcmp(str, "event"     ) &&
        strcmp(str, "background") &&
        !config.allow_unknown       )
   {
       print_error("invalid class '%s'", str);
   }

    return str;
}

static uint32_t parse_resource_list(char *rlist_str, int exit_if_error)
{
    static rdef_t  rdef[] = {
        { RESMSG_AUDIO_PLAYBACK ,  "AudioPlayback"  },
        { RESMSG_VIDEO_PLAYBACK ,  "VideoPlayback"  },
        { RESMSG_AUDIO_RECORDING,  "AudioRecording" },
        { RESMSG_VIDEO_RECORDING,  "VideoRecording" },
        { RESMSG_VIBRA          ,  "Vibra"          },
        { RESMSG_LEDS           ,  "Leds"           },
        { RESMSG_BACKLIGHT      ,  "BackLight"      },
        { RESMSG_SYSTEM_BUTTON  ,  "SystemButton"   },
        { RESMSG_LOCK_BUTTON    ,  "LockButton"     },
        { RESMSG_SCALE_BUTTON   ,  "ScaleButton"    },
        { RESMSG_SNAP_BUTTON    ,  "SnapButton"     },
        { RESMSG_LENS_COVER     ,  "LensCover"      },
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

static uint32_t parse_mode_values(char *mval_str, int exit_if_error)
{
    static rdef_t  mdef[] = {
        { RESMSG_MODE_AUTO_RELEASE ,  "AutoRelease"  },
        {             0            ,       NULL      }
    };

    uint32_t  mode = 0;
    char      buf[256];
    rdef_t   *md;
    char     *p;
    char     *e;

    if (mval_str == NULL)
        print_message("missing mode values");
    else {
        strncpy(buf, mval_str, sizeof(buf));
        buf[sizeof(buf)-1] = '\0';

        p = buf;

        do {
            if ((e = strchr(p, ',')) != NULL)
                *e++ = '\0';

            for (md = mdef;  md->name != NULL;  md++) {
                if (!strcmp(p, md->name))
                    break;
            }

            if (!md->value) {
                print_message("invalid resource values '%s'", mval_str);
                mode = 0;
                break;
            }

            mode |= md->value;

        } while((p = e) != NULL);
    }

    if (!mode && exit_if_error)
        exit(EINVAL);

    return mode;
}

static DBusBusType parse_bustype(char *bustype_str, int exit_if_error)
{
    DBusBusType bustype = DBUS_BUS_SYSTEM;

    if (!strcmp(bustype_str, "session"))
        bustype = DBUS_BUS_SESSION;
    else if (strcmp(bustype_str, "system")) {
        print_message("invalid D-Bus type '%s'", bustype_str);

        if (exit_if_error)
            exit(EINVAL);
    }

    return bustype;
}


/*
 * Local Variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
