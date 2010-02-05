#ifndef __RES_TYPES_H__
#define __RES_TYPES_H__

#define RESOURCE_BIT(b)   (((uint32_t)1) << (b))

#ifdef	__cplusplus
extern "C" {
#endif

typedef enum {
    resource_audio_playback  = 0,
    resource_video_playback  = 1,
    resource_audio_recording = 2,
    resource_video_recording = 3,
    resource_vibra           = 4,
    resource_leds            = 5,
    resource_backlight       = 6,
    resource_system_button   = 8,
    resource_lock_button     = 9,
    resource_scale_button    = 10,
    resource_snap_button     = 11,
    resource_lens_cover      = 12,
} resource_type_t;


#define RESOURCE_AUDIO_PLAYBACK   RESOURCE_BIT( resource_audio_playback )
#define RESOURCE_VIDEO_PLAYBACK   RESOURCE_BIT( resource_video_playback )
#define RESOURCE_AUDIO_RECORDING  RESOURCE_BIT( resource_audio_recording )
#define RESOURCE_VIDEO_RECORDING  RESOURCE_BIT( resource_video_recording )
#define RESOURCE_VIBRA            RESOURCE_BIT( resource_vibra )
#define RESOURCE_LEDS             RESOURCE_BIT( resource_leds )
#define RESOURCE_BACKLIGHT        RESOURCE_BIT( resource_backlight )
#define RESOURCE_SYSTEM_BUTTON    RESOURCE_BIT( resource_system_button )
#define RESOURCE_LOCK_BUTTON      RESOURCE_BIT( resource_lock_button )
#define RESOURCE_SCALE_BUTTON     RESOURCE_BIT( resource_scale_button )
#define RESOURCE_SNAP_BUTTON      RESOURCE_BIT( resource_snap_button )
#define RESOURCE_LENS_COVER       RESOURCE_BIT( resource_lens_cover )

typedef enum {
  resource_auto_release = 0,
  resource_always_reply = 1,
} resource_mode_t;

#define RESOURCE_AUTO_RELEASE     RESOURCE_BIT( resource_auto_release )
#define RESOURCE_ALWAYS_REPLY     RESOURCE_BIT( resource_always_reply )


#ifdef	__cplusplus
};
#endif


#endif	/* __RES_TYPES_H__ */

/* 
 * Local Variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim:set expandtab shiftwidth=4:
 */