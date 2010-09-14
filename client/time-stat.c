#include <math.h>
#include "time-stat.h"

static struct timespec start_time;

int start_timer(void)
{
    int r;
    r = clock_gettime(CLOCK_REALTIME, &start_time);

    if (r == 0)
        return 1;
    else
        return 0;
}

long int stop_timer(void)
{
    struct timespec end_time;
    int r;
    double milliseconds = 0.0;

    r = clock_gettime(CLOCK_REALTIME, &end_time);

    if (r == 0) {
        milliseconds = 1000.0 * (end_time.tv_sec - start_time.tv_sec) +
                   (end_time.tv_nsec - start_time.tv_nsec) / 1000000.0;
    }
    start_time.tv_sec = 0;
    start_time.tv_nsec = 0;

    return lround(milliseconds);
}

