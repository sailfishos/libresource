#include <stdlib.h>
#include <stdio.h>

#include <glib.h>

#include <resource.h>

static GMainLoop       *main_loop;


static void create_mainloop (void);
static void destroy_mainloop (void);
static void run_mainloop (void);

static void grant_callback (resource_set_t *, uint32_t, void *);
static void advice_callback (resource_set_t *, uint32_t, void *);


int main (int argc, char **atgv)
{
  resource_set_t *resource_set;

  create_mainloop ();

  resource_set = resource_set_create ("player", RESOURCE_AUDIO_PLAYBACK,0, 0,
                                      grant_callback, NULL);
  resource_set_configure_advice_callback (resource_set, advice_callback, NULL);
  resource_set_configure_audio (resource_set, "fmradio", 0,NULL);
  resource_set_acquire (resource_set);

  run_mainloop ();

  resource_set_destroy (resource_set);

  destroy_mainloop ();
  

}


static void create_mainloop (void)
{
    if ((main_loop = g_main_loop_new (NULL, FALSE)) == NULL) {
        printf("Can't create G-MainLoop\n");
    }
}

static void destroy_mainloop (void)
{
    g_main_loop_unref (main_loop);
}

static void run_mainloop (void)
{
    g_main_loop_run (main_loop);
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


/*
 * Local Variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
