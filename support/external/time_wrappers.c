// NOLINTBEGIN(misc-use-internal-linkage)

/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 19/08/2026
 */

#include <time.h>

// ---------------------------------------------------------
// time family wrappers
// ---------------------------------------------------------
time_t mktimep(struct tm* arg)
{
    return mktime(arg);
}

float difftimep(time_t time_end, time_t time_beg)
{
    return (float) difftime(time_end, time_beg);
}

