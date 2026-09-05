// NOLINTBEGIN(misc-use-internal-linkage)

/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 21/08/2026
 */

#include <time.h>

// ---------------------------------------------------------
// time manipulation family wrappers
// ---------------------------------------------------------
double _c66_common_difftime(time_t time_end, time_t time_beg)
{
    return (float)difftime(time_end, time_beg);
}

time_t _c66_common_time(time_t* arg)
{
    return time(arg);
}

int _c66_common_clock(void)
{
    return (int)clock();
}

// ---------------------------------------------------------
// format conversion family wrappers
// ---------------------------------------------------------
char* _c66_common_asctime(const struct tm* time_ptr)
{
    return asctime(time_ptr);
}

char* _c66_common_ctime(const time_t* timer)
{
    return ctime(timer);
}

size_t _c66_common_strftime(char* str, size_t count, const char* format, const struct tm* tp)
{
    return strftime(str, count, format, tp);
}

struct tm* _c66_common_gmtime(const time_t* timer)
{
    return gmtime(timer);
}

struct tm* _c66_common_localtime(const time_t* timer)
{
    return localtime(timer);
}

time_t _c66_common_mktime(struct tm* arg)
{
    return mktime(arg);
}