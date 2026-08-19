// NOLINTBEGIN(misc-use-internal-linkage)


/*
* Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 15/08/2026
 */


#include <string.h>

// ---------------------------------------------------------
// Memory primitive wrappers
// ---------------------------------------------------------

void* memcpyp(void* dest, const void* src, const size_t n) { return memcpy(dest, src, n); }
void* memmovep(void* dest, const void* src, const size_t n) { return memmove(dest, src, n); }
int memcmpp(const void* s1, const void* s2, const size_t n) { return memcmp(s1, s2, n); }
void* memchrp(const void* s, const int c, const size_t n) { return (void*)memchr(s, c, n); }
void* memsetp(void* s, const int c, const size_t n) { return memset(s, c, n); }

// ---------------------------------------------------------
// String manipulation wrappers
// ---------------------------------------------------------
char* strcpyp(char* dest, const char* src) { return strcpy(dest, src); }
char* strncpyp(char* dest, const char* src, const size_t n) { return strncpy(dest, src, n); }
char* strcatp(char* dest, const char* src) { return strcat(dest, src); }
char* strncatp(char* dest, const char* src, const size_t n) { return strncat(dest, src, n); }
int strcmpp(const char* s1, const char* s2) { return strcmp(s1, s2); }
int strncmpp(const char* s1, const char* s2, const size_t n) { return strncmp(s1, s2, n); }
char* strchrp(const char* s, const int c) { return (char*)strchr(s, c); }
char* strrchrp(const char* s, const int c) { return (char*)strrchr(s, c); }
size_t strspnp(const char* s, const char* accept) { return strspn(s, accept); }
size_t strcspnp(const char* s, const char* reject) { return strcspn(s, reject); }
char* strpbrkp(const char* s, const char* accept) { return (char*)strpbrk(s, accept); }
char* strstrp(const char* haystack, const char* needle) { return (char*)strstr(haystack, needle); }
size_t strlenp(const char* s) { return strlen(s); }
char* strtokp(char* str, const char* delim) { return strtok(str, delim); }
char* strerrorp(const int errnum) { return strerror(errnum); }

