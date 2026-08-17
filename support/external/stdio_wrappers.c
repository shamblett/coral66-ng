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
int printfintp(const char* fmt, const int arg)
{
    return printf(fmt, arg);
}

int printfstrp(const char* fmt, const char* arg)
{
    return printf(fmt, arg);
}

int printf2intp(const char* fmt, const int arg1, const int arg2)
{
    return printf(fmt, arg1, arg2);
}

// ---------------------------------------------------------
// fprintf family wrappers
// ---------------------------------------------------------
int fprintfintp(FILE* stream, const char* fmt, const int arg)
{
    return fprintf(stream, fmt, arg);
}

int fprintfstrp(FILE* stream, const char* fmt, const char* arg)
{
    return fprintf(stream, fmt, arg);
}

// ---------------------------------------------------------
// sprintf family wrappers
// ---------------------------------------------------------
int sprintfintp(char* str, const char* fmt, const int arg)
{
    return sprintf(str, fmt, arg);
}

int sprintfstrp(char* str, const char* fmt, const char* arg)
{
    return sprintf(str, fmt, arg);
}

// ---------------------------------------------------------
// scanf family wrappers
// ---------------------------------------------------------
int scanfintp(const char* fmt, int* arg)
{
    return scanf(fmt, arg);
}

int scanfstrp(const char* fmt, char* arg)
{
    return scanf(fmt, arg);
}

// ---------------------------------------------------------
// fscanf family wrappers
// ---------------------------------------------------------
int fscanfintp(FILE* stream, const char* fmt, int* arg)
{
    return fscanf(stream, fmt, arg);
}

int fscanfstrp(FILE* stream, const char* fmt, char* arg)
{
    return fscanf(stream, fmt, arg);
}

// ---------------------------------------------------------
// File Access wrappers
// ---------------------------------------------------------
FILE* fopenp(const char* filename, const char* mode)
{
    return fopen(filename, mode);
}

int fclosep(FILE* stream)
{
    return fclose(stream);
}

int fflushp(FILE* stream)
{
    return fflush(stream);
}

void setbufp(FILE* stream, char* buffer)
{
    setbuf(stream, buffer);
}

// ---------------------------------------------------------
// Direct Input/Output wrappers
// ---------------------------------------------------------
size_t freadp(void* ptr, const size_t size, const size_t nmemb, FILE* stream)
{
    return fread(ptr, size, nmemb, stream);
}

size_t fwritep(const void* ptr, const size_t size, const size_t nmemb, FILE* stream)
{
    return fwrite(ptr, size, nmemb, stream);
}

// ---------------------------------------------------------
// Character and String I/O wrappers
// ---------------------------------------------------------
int fgetcp(FILE* stream)
{
    return fgetc(stream);
}

char* fgetsp(char* str, const int n, FILE* stream)
{
    return fgets(str, n, stream);
}

int fputcp(const int c, FILE* stream)
{
    return fputc(c, stream);
}

int fputsp(const char* str, FILE* stream)
{
    return fputs(str, stream);
}

int getcharp(void)
{
    return getchar();
}

char* getsp(char* str)
{
    return fgets(str, 1024, stdin);
}

int putcharp(const int c)
{
    return putchar(c);
}

int putsp(const char* str)
{
    return puts(str);
}

int ungetcp(const int c, FILE* stream)
{
    return ungetc(c, stream);
}

// ---------------------------------------------------------
// File Positioning wrappers
// ---------------------------------------------------------
long ftellp(FILE* stream)
{
    return ftell(stream);
}

int fgetposp(FILE* stream, fpos_t* pos)
{
    return fgetpos(stream, pos);
}

int fseekp(FILE* stream, const long offset, const int whence)
{
    return fseek(stream, offset, whence);
}

int fsetposp(FILE* stream, const fpos_t* pos)
{
    return fsetpos(stream, pos);
}

void rewindp(FILE* stream)
{
    rewind(stream);
}

// ---------------------------------------------------------
// File Operations wrappers
// ---------------------------------------------------------
int removep(const char* filename)
{
    return remove(filename);
}

int renamep(const char* oldfilename, const char* newfilename)
{
    return rename(oldfilename, newfilename);
}

#pragma GCC diagnostic pop
