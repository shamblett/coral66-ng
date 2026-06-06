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

// LOCATION INTEGER
// EXTERNAL ( PROCEDURE PrintValue(LOCATION INTEGER) );
void PrintValue(const int val)
{
    printf("%d\n", val);
}

// EXTERNAL ( PROCEDURE Exit(INTEGER) );
void Exit(const int code)
{
    exit(code);
}

// The signature must match the name used in your Coral EXTERNAL declaration
//
// // We use intptr_t to safely hold the memory address regardless of whether
// the platform is 32-bit or 64-bit.
// VALUE INTEGER
// EXTERNAL ( PROCEDURE PrintString(VALUE INTEGER) );

void PrintString(const intptr_t stringAddress)
{
    // Cast the raw integer back to a standard C character pointer
    const auto str = (const char*)stringAddress;

    // Call the standard library function
    printf("%s\n", str);
}

// Optional: For printing integers as you requested earlier
// VALUE INTEGER
// EXTERNAL ( PROCEDURE PrintInteger(VALUE INTEGER) );

void PrintInteger(const intptr_t val)
{
    printf("%d\n", (int)val);
}
