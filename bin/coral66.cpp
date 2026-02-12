/*
* Project: coral66
* Author : Copyright S. Hamblett <steve.hamblett@linux.com>
* Date   : 31/01/2026
*/

#include <string_view>
#include <vector>

#include "lib/main/main.hpp"

int main(const int argc, char** argv)
{
    return coral66::main(argc, argv);
}

#ifdef __clang__

extern "C" int lprofGetHostName(char* Name, int Len)
{
    WCHAR Buffer[128];
    DWORD BufferSize = sizeof(Buffer);
    BOOL Result = GetComputerNameExW(ComputerNameDnsFullyQualified, Buffer, &BufferSize);
    if (!Result)
        return -1;
    if (WideCharToMultiByte(CP_UTF8, 0, Buffer, -1, Name, Len, nullptr, nullptr) == 0)
        return -1;
    return 0;
}

#endif