// NOLINTBEGIN(misc-use-internal-linkage)

/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 25/08/2026
 */

#include <pwd.h>
#include <stdlib.h>
#include <unistd.h>

// ---------------------------------------------------------
// process information
// ---------------------------------------------------------
char* _c66_common_cuserid(void)
{
    const struct passwd* pws = getpwuid(geteuid());
    return pws->pw_name;
}

int _c66_common_getegid(void)
{
    return (int)getegid();
}

int _c66_common_geteuid(void)
{
    return (int)geteuid();
}

int _c66_common_getgid(void)
{
    return (int)getgid();
}

int _c66_common_getpgid(const int pid)
{
    return getpgid(pid);
}

int _c66_common_getpgrp(void)
{
    return getpgrp();
}

int _c66_common_getpid(void)
{
    return getpid();
}

int _c66_common_getppid(void)
{
    return getppid();
}

int _c66_common_getsid(const int pid)
{
    return getsid(pid);
}

int _c66_common_getuid(void)
{
    return getuid();
}

// ---------------------------------------------------------
// process control
// ---------------------------------------------------------
void _c66_common_exit(const int code)
{
    exit(code);
}

void _c66_common_sleep(const int seconds)
{
    sleep(seconds);
}