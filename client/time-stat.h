#ifndef TIME_STAT_H
#define TIME_STAT_H
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif
int start_timer(void);

long int stop_timer(void);
#ifdef __cplusplus
}
#endif

#endif

