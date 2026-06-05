#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// LOCATION INTEGER
// EXTERNAL ( PROCEDURE CoralPutsValue(LOCATION INTEGER) );
void CoralPutsValue(int val)
{
    printf("%d\n", val);
}

// EXTERNAL ( PROCEDURE CoralExit(INTEGER) );
void CoralExit(int code)
{
    exit(code);
}

// The signature must match the name used in your Coral EXTERNAL declaration
//
// // We use intptr_t to safely hold the memory address regardless of whether
// the platform is 32-bit or 64-bit.
// VALUE INTEGER
// EXTERNAL ( 'PROCEDURE' CoralPutsString('VALUE' 'INTEGER') );

void CoralPutsString(intptr_t stringAddress)
{
    // Cast the raw integer back to a standard C character pointer
    const char* str = (const char*)stringAddress;

    // Call the standard library function
    printf("%s\n", str);
}

// Optional: For printing integers as you requested earlier
// VALUE INTEGER
// 'EXTERNAL' ( 'PROCEDURE' CoralPrintInteger('VALUE' 'INTEGER') );

void CoralPrintInteger(intptr_t val)
{
    printf("%d\n", (int)val);
}
