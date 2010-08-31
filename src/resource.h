#ifndef __LIB_RESOURCE_H__
#define __LIB_RESOURCE_H__

#include <stdint.h>
#include <sys/types.h>
#include <res-types.h>


#ifdef	__cplusplus
extern "C" {
#endif

typedef struct resource_set_s resource_set_t;


typedef void (*resource_callback_t)(resource_set_t *resource_set,
                                    uint32_t        resources,
                                    void           *userdata);

typedef void (*error_callback_function_t)(resource_set_t *resource_set,
                                          uint32_t        errcod,
                                          const char     *errmsg,
                                          void           *userdata);

resource_set_t *resource_set_create(const char          *klass,
                                    uint32_t             mandatory,
                                    uint32_t             optional,
                                    uint32_t             mode,
                                    resource_callback_t  grantcb,
                                    void                *grantdata);

void resource_set_destroy(resource_set_t *resource_set);

int  resource_set_configure_advice_callback(resource_set_t      *resource_set,
                                            resource_callback_t  advicecb,
                                            void                *advicedata);

int resource_set_configure_error_callback(resource_set_t             *rs,
                                          error_callback_function_t  errorcb,
                                          void                      *errordata);

int  resource_set_configure_resources(resource_set_t *resource_set,
                                      uint32_t        mandatory,
                                      uint32_t        optional);

int  resource_set_configure_audio(resource_set_t *resource_set,
                                  const char     *audio_group,
                                  pid_t           pid_of_renderer,
                                  const char     *pulseaudio_stream_name);

int  resource_set_acquire(resource_set_t *resource_set);
int  resource_set_release(resource_set_t *resource_set);




#ifdef	__cplusplus
};
#endif


#endif /* __LIB_RESOURCE_H__ */

/* 
 * Local Variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim:set expandtab shiftwidth=4:
 */
