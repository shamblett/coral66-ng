// NOLINTBEGIN(misc-use-internal-linkage)

/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 13/08/2026
 */

#include <stdio.h>

// ---------------------------------------------------------
// printf family wrappers
// ---------------------------------------------------------
int _c66_common_printfint(const char* fmt, const int arg)
{
    return printf(fmt, arg);
}

int _c66_common_printfstr(const char* fmt, const char* arg)
{
    return printf(fmt, arg);
}

int _c66_common_printf2int(const char* fmt, const int arg1, const int arg2)
{
    return printf(fmt, arg1, arg2);
}

// ---------------------------------------------------------
// fprintf family wrappers
// ---------------------------------------------------------
int _c66_common_fprintfint(FILE* stream, const char* fmt, const int arg)
{
    return fprintf(stream, fmt, arg);
}

int _c66_common_fprintfstr(FILE* stream, const char* fmt, const char* arg)
{
    return fprintf(stream, fmt, arg);
}

// ---------------------------------------------------------
// sprintf family wrappers
// ---------------------------------------------------------
int _c66_common_sprintfint(char* str, const char* fmt, const int arg)
{
    return sprintf(str, fmt, arg);
}

int _c66_common_sprintfstr(char* str, const char* fmt, const char* arg)
{
    return sprintf(str, fmt, arg);
}

// ---------------------------------------------------------
// scanf family wrappers
// ---------------------------------------------------------
int _c66_common_scanfint(const char* fmt, int* arg)
{
    return scanf(fmt, arg);
}

int _c66_common_scanfstr(const char* fmt, char* arg)
{
    return scanf(fmt, arg);
}

// ---------------------------------------------------------
// fscanf family wrappers
// ---------------------------------------------------------
int _c66_common_fscanfint(FILE* stream, const char* fmt, int* arg)
{
    return fscanf(stream, fmt, arg);
}

int _c66_common_fscanfstr(FILE* stream, const char* fmt, char* arg)
{
    return fscanf(stream, fmt, arg);
}

// ---------------------------------------------------------
// File Access wrappers
// ---------------------------------------------------------
FILE* _c66_common_fopen(const char* filename, const char* mode)
{
    return fopen(filename, mode);
}

int _c66_common_fclose(FILE* stream)
{
    return fclose(stream);
}

int _c66_common_fflush(FILE* stream)
{
    return fflush(stream);
}

void _c66_common_setbuf(FILE* stream, char* buffer)
{
    setbuf(stream, buffer);
}

// ---------------------------------------------------------
// Direct Input/Output wrappers
// ---------------------------------------------------------
size_t _c66_common_fread(void* ptr, const size_t size, const size_t nmemb, FILE* stream)
{
    return fread(ptr, size, nmemb, stream);
}

size_t _c66_common_fwrite(const void* ptr, const size_t size, const size_t nmemb, FILE* stream)
{
    return fwrite(ptr, size, nmemb, stream);
}

// ---------------------------------------------------------
// Character and String I/O wrappers
// ---------------------------------------------------------
int _c66_common_fgetc(FILE* stream)
{
    return fgetc(stream);
}

char* _c66_common_fgets(char* str, const int n, FILE* stream)
{
    return fgets(str, n, stream);
}

int _c66_common_fputc(const int c, FILE* stream)
{
    return fputc(c, stream);
}

int _c66_common_fputs(const char* str, FILE* stream)
{
    return fputs(str, stream);
}

int _c66_common_getchar(void)
{
    return getchar();
}

char* _c66_common_gets(char* str)
{
    return fgets(str, 1024, stdin);
}

int _c66_common_putchar(const int c)
{
    return putchar(c);
}

int _c66_common_puts(const char* str)
{
    return puts(str);
}

int _c66_common_ungetc(const int c, FILE* stream)
{
    return ungetc(c, stream);
}

// ---------------------------------------------------------
// File Positioning wrappers
// ---------------------------------------------------------
long _c66_common_ftell(FILE* stream)
{
    return ftell(stream);
}

int _c66_common_fgetpos(FILE* stream, fpos_t* pos)
{
    return fgetpos(stream, pos);
}

int _c66_common_fseek(FILE* stream, const long offset, const int whence)
{
    return fseek(stream, offset, whence);
}

int _c66_common_fsetpos(FILE* stream, const fpos_t* pos)
{
    return fsetpos(stream, pos);
}

void _c66_common_rewind(FILE* stream)
{
    rewind(stream);
}

// ---------------------------------------------------------
// File Operations wrappers
// ---------------------------------------------------------
int _c66_common_remove(const char* filename)
{
    return remove(filename);
}

int _c66_common_rename(const char* oldfilename, const char* newfilename)
{
    return rename(oldfilename, newfilename);
}

