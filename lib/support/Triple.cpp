/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "Triple.hpp"
#include "Utilities.hpp"

coral66::Triple::Triple(coral66::Architecture const architecture, coral66::Platform const operatingSystem, coral66::Environment const environment)
    : m_architecture(architecture), m_operatingSystem(operatingSystem), m_environment(environment)
{
}

coral66::Triple coral66::Triple::defaultTarget()
{
#ifdef CLD_DEFAULT_TARGET
    static auto target = fromString(CLD_DEFAULT_TARGET);
    return target;
#endif
    return native();
}

coral66::Triple coral66::Triple::native()
{
 // ReSharper disable once CppDFAUnusedValue
    Architecture architecture = Architecture ::Unknown;
#if defined(_M_AMD64) || defined(__x86_64__)
    architecture = Architecture::x86_64;
#endif

 // ReSharper disable once CppDFAUnusedValue
    Platform operatingSystem = Platform::Unknown;
#if defined(__linux__)
    operatingSystem = Platform ::Linux;
#endif

 // ReSharper disable once CppDFAUnusedValue
    Environment environment = Environment ::Unknown;
#if defined(__GNUC__)
    environment = Environment ::GNU;
#endif
    return {architecture, operatingSystem, environment};
}

coral66::Triple coral66::Triple::fromString(std::string_view string)
{
    const auto first = string.substr(0, string.find('-'));
    string.remove_prefix(std::min(first.size() + 1, string.size()));
    const auto second = string.substr(0, string.find('-'));
    string.remove_prefix(std::min(second.size() + 1, string.size()));
    const auto third = string.substr(0, string.find('-'));
    string.remove_prefix(std::min(third.size() + 1, string.size()));
    auto  architecture = Architecture::Unknown;
    if (first == "x86_64")
    {
        architecture = Architecture::x86_64;
    }
    auto  platform = Platform::Unknown;
    if (second == "linux")
    {
        platform = Platform::Linux;
    }
    auto  environment = Environment::Unknown;
    if (third == "gnu")
    {
        environment = Environment::GNU;
    }
    return {architecture, platform, environment};
}
