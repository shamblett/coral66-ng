/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

//
// Externals wrapper to allow communication between the EXTERNAL communicator declarations and
// the platform library facilities.
// The procedure signature must match the name and parameters used in your Coral EXTERNAL
// communicator declaration,

// EXTERNAL ( PROCEDURE Exit(INTEGER) );
// Exits the program with the specified exit code.
void Exit(const int code)
{
    exit(code);
}

// EXTERNAL ( PROCEDURE NewLines(INTEGER) );
// Prints the specified number of newlines
void NewLines(const int number)
{
    for (int i = 0; i <= number; i++)
    {
        putchar('\n');
    }
}

// EXTERNAL ( PROCEDURE PrintString(VALUE INTEGER) );
// Prints the supplied string, no newline.
void PrintString(const intptr_t stringAddress)
{
    // Cast the raw integer back to a standard C character pointer
    const auto str = (const char*)stringAddress;

    // Call the standard library function
    printf("%s", str);
}

// EXTERNAL ( PROCEDURE PrintInteger(VALUE INTEGER) );
// Prints the supplied integer, no newline
void PrintInteger(const intptr_t val)
{
    printf("%d", (int)val);
}
