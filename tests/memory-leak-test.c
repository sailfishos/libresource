#include <dbus/dbus.h>
#include <stdlib.h>
#include <stdio.h>

#include <glib.h>

#include <resource.h>
#include <res-msg.h>

static GMainLoop       *main_loop;

static int iterations;

static gboolean destructor (void *data);

static void grant_callback (resource_set_t *resource_set,
                            uint32_t        resources,
                            void           *userdata)
{
    char buf[512];

    (void)resource_set;
    (void)userdata;

    printf("*** %s(): granted resources %s\n", __FUNCTION__,
           resmsg_res_str (resources, buf, sizeof(buf)));


    if (resources != 0) {
      resource_set_release(resource_set);

      iterations--;

      if (iterations <= 0) {
        destructor(0);
      }
    } else {
      resource_set_acquire(resource_set);
    }



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

static gboolean destructor (void *data)
{
  printf("destructor\n");
    resource_set_t *resource_set = data;

    g_main_loop_quit(main_loop);

    return FALSE;
}

static void schedule_destruction (resource_set_t *resource_set, guint ms)
{
  printf("schedule_destruction\n");
    g_timeout_add (ms, destructor, resource_set);
}

static void create_mainloop (void)
{
  printf("create_mainloop\n");

    if ((main_loop = g_main_loop_new (NULL, FALSE)) == NULL) {
        printf("Can't create G-MainLoop\n");
    }
}

static void destroy_mainloop (void)
{
  printf("destroy_mainloop\n");

  g_main_loop_unref (main_loop);
}

static void run_mainloop (void)
{
  printf("run_mainloop\n");

  g_main_loop_run (main_loop);
}




int main(int argc, char* argv[]) {
  resource_set_t *rs;
  create_mainloop ();

  rs = resource_set_create("player", RESOURCE_AUDIO_PLAYBACK, 0, 0, grant_callback, 0);

  resource_set_configure_advice_callback (rs, advice_callback, NULL);
  resource_set_configure_audio (rs, "fmradio", 0, NULL);

  iterations = 10;
  if (argc > 1)  iterations = atoi(argv[1]);

  resource_set_acquire(rs);
  run_mainloop();

  destroy_mainloop ();
}
