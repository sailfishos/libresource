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
#include <glib.h>

#include "../src/resource.h"

static void grant_callback (resource_set_t *resource_set,
                            uint32_t        resources,
                            void           *userdata)
{
    char buf[512];

    (void)resource_set;
    (void)userdata;

    printf("*** %s(): granted resources %s\n", __FUNCTION__,
           resmsg_res_str (resources, buf, sizeof(buf)));

    exit(0);
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

static void error_callback (resource_set_t *resource_set,
                            uint32_t        errcod,
                            const char*     errmsg,
                            void           *userdata)
{
    (void)resource_set;
    (void)userdata;

    printf("*** %s(): error callback code: %d msg: %s\n", __FUNCTION__,
           errcod, errmsg);

    exit(1);
}

int
main(int argc, char* argv[]) {

  resource_set_t *rs;
  GMainLoop      *main_loop;

  rs = resource_set_create("call", RESOURCE_AUDIO_PLAYBACK, 0, 0, grant_callback, 0);
  resource_set_configure_advice_callback (rs, advice_callback, NULL);
  resource_set_configure_error_callback (rs, error_callback, NULL);

  resource_set_acquire(rs);


  if ((main_loop = g_main_loop_new (NULL, FALSE)) == NULL) {
      printf("Can't create G-MainLoop\n");
  }

  g_main_loop_run (main_loop);

  return 0;
}


