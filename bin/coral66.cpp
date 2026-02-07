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
    std::vector<std::string_view> elements(argc - 1);
    for (int i = 1; i < argc; i++)
    {
        elements[i - 1] = argv[i];
    }
    return coral66::main(elements);
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