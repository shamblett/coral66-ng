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
double difftimep(time_t time_end, time_t time_beg)
{
    return (float)difftime(time_end, time_beg);
}

time_t timep(time_t* arg)
{
    return time(arg);
}

int clockp(void)
{
    return (int)clock();
}

// ---------------------------------------------------------
// format conversion family wrappers
// ---------------------------------------------------------
char* asctimep(const struct tm* time_ptr)
{
    return asctime(time_ptr);
}

char* ctimep(const time_t* timer)
{
    return ctime(timer);
}

size_t strftimep(char* str, size_t count, const char* format, const struct tm* tp)
{
    return strftime(str, count, format, tp);
}

struct tm* gmtimep(const time_t* timer)
{
    return gmtime(timer);
}

struct tm* localtimep(const time_t* timer)
{
    return localtime(timer);
}

time_t mktimep(struct tm* arg)
{
    return mktime(arg);
}