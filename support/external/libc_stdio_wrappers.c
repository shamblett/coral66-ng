/*
* Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 13/08/2026
 */

#include <stdio.h>

// ---------------------------------------------------------
// printf family wrappers
// ---------------------------------------------------------
int printfintp(const char* fmt, int arg) {
    return printf(fmt, arg);
}

int printfstrp(const char* fmt, const char* arg) {
    return printf(fmt, arg);
}

int printf2intp(const char* fmt, int arg1, int arg2) {
    return printf(fmt, arg1, arg2);
}

// ---------------------------------------------------------
// fprintf family wrappers
// ---------------------------------------------------------
int fprintfintp(FILE* stream, const char* fmt, int arg) {
    return fprintf(stream, fmt, arg);
}

int fprintfstrp(FILE* stream, const char* fmt, const char* arg) {
    return fprintf(stream, fmt, arg);
}

// ---------------------------------------------------------
// sprintf family wrappers
// ---------------------------------------------------------
int sprintfintp(char* str, const char* fmt, int arg) {
    return sprintf(str, fmt, arg);
}

int sprintfstrp(char* str, const char* fmt, const char* arg) {
    return sprintf(str, fmt, arg);
}

// ---------------------------------------------------------
// scanf family wrappers
// ---------------------------------------------------------
int scanfintp(const char* fmt, int* arg) {
    return scanf(fmt, arg);
}

int scanfstrp(const char* fmt, char* arg) {
    return scanf(fmt, arg);
}

// ---------------------------------------------------------
// fscanf family wrappers
// ---------------------------------------------------------
int fscanfintp(FILE* stream, const char* fmt, int* arg) {
    return fscanf(stream, fmt, arg);
}

int fscanfstrp(FILE* stream, const char* fmt, char* arg) {
    return fscanf(stream, fmt, arg);
}
