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
char* cuseridp(void)
{
    struct passwd* pws = getpwuid(geteuid());
    return pws->pw_name;
}

int getegidp(void)
{
    return (int)getegid();
}

int geteuidp(void)
{
    return (int)geteuid();
}

int getgidp(void)
{
    return (int)getgid();
}

int getpgidp(int pid)
{
    return getpgid(pid);
}

int getpgrpp(void)
{
    return getpgrp();
}

int getpidp(void)
{
    return getpid();
}

int getppidp(void)
{
    return getppid();
}

int getsidp(int pid)
{
    return getsid(pid);
}

int getuidp(void)
{
    return getuid();
}

// ---------------------------------------------------------
// process control
// ---------------------------------------------------------
void exitp(const int code)
{
    exit(code);
}

void sleepp(int seconds)
{
    sleep(seconds);
}