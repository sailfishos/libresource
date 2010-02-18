#include "resource.h"

resource_set_t *resource_set_create(const char          *klass,
                                    uint32_t             mandatory,
                                    uint32_t             optional,
                                    uint32_t             mode,
                                    resource_callback_t  grantcb,
                                    void                *grantdata);

void resource_set_destroy(resource_set_t *resource_set);

int  resource_set_configure_advice_callback(resource_set_t      *resource_set,
                                            resource_callback_t *advicecb,
                                            void                *advicedata);

int  resource_set_configure_resources(resource_set_t *resource_set,
                                      uint32_t        mandatory,
                                      uint32_t        optional);

int  resource_set_configure_audio(resource_set_t *resource_set,
                                  const char     *audio_group,
                                  pid_t           pid_of_renderer,
                                  const char     *pulseaudio_stream_name);

int  resource_set_acquire(resource_set_t *resource_set);
int  resource_set_release(resource_set_t *resource_set);

int main() {
	resource_callback_t callback;
}
