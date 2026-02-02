/*
* Project: coral66
* Author : Copyright S. Hamblett <steve.hamblett@linux.com>
* Date   : 31/01/2026
*/

#include "main/Main.hpp"

#include <string_view>
#include <vector>

int main(const int argc, char** argv)
{
    std::vector<std::string_view> elements(argc - 1);
    for (int i = 1; i < argc; i++)
    {
        elements[i - 1] = argv[i];
    }
    return coral66::main(elements);
}
