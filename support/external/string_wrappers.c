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

void* _c66_common_memcpy(void* dest, const void* src, const size_t n) { return memcpy(dest, src, n); }
void* _c66_common_memmove(void* dest, const void* src, const size_t n) { return memmove(dest, src, n); }
int _c66_common_memcmp(const void* s1, const void* s2, const size_t n) { return memcmp(s1, s2, n); }
void* _c66_common_memchr(const void* s, const int c, const size_t n) { return (void*)memchr(s, c, n); }
void* _c66_common_memset(void* s, const int c, const size_t n) { return memset(s, c, n); }

// ---------------------------------------------------------
// String manipulation wrappers
// ---------------------------------------------------------
char* _c66_common_strcpy(char* dest, const char* src) { return strcpy(dest, src); }
char* _c66_common_strncpy(char* dest, const char* src, const size_t n) { return strncpy(dest, src, n); }
char* _c66_common_strcat(char* dest, const char* src) { return strcat(dest, src); }
char* _c66_common_strncat(char* dest, const char* src, const size_t n) { return strncat(dest, src, n); }
int _c66_common_strcmp(const char* s1, const char* s2) { return strcmp(s1, s2); }
int _c66_common_strncmp(const char* s1, const char* s2, const size_t n) { return strncmp(s1, s2, n); }
char* _c66_common_strchr(const char* s, const int c) { return (char*)strchr(s, c); }
char* _c66_common_strrchr(const char* s, const int c) { return (char*)strrchr(s, c); }
size_t _c66_common_strspn(const char* s, const char* accept) { return strspn(s, accept); }
size_t _c66_common_strcspn(const char* s, const char* reject) { return strcspn(s, reject); }
char* _c66_common_strpbrk(const char* s, const char* accept) { return (char*)strpbrk(s, accept); }
char* _c66_common_strstr(const char* haystack, const char* needle) { return (char*)strstr(haystack, needle); }
size_t _c66_common_strlen(const char* s) { return strlen(s); }
char* _c66_common_strtok(char* str, const char* delim) { return strtok(str, delim); }
char* _c66_common_strerror(const int errnum) { return strerror(errnum); }

