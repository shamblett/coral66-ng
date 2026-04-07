/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "test/common/test_config.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

int main(int argc, char* argv[])
{
    auto enable = enableParallelization();

    Catch::Session session; // There must be exactly one instance
    // writing to session.configData() here sets defaults
    // this is the preferred way to set them

    // Set the run order as declaration order
    session.configData().runOrder = Catch::TestRunOrder::Declared;

    if (int returnCode = session.applyCommandLine(argc, argv); returnCode != 0) // Indicates a command line error
        return returnCode;

    // writing to session.configData() or session.Config() here
    // overrides command line args
    // only do this if you know you need to

    const int numFailed = session.run();

    // numFailed is clamped to 255 as some unices only use the lower 8 bits.
    // This clamping has already been applied, so just return it here
    // You can also do any post run clean-up here
    return numFailed;
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
