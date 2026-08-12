/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

/**
 * Architecture support.
 *
 * Note that we only cater for x86_64 GNU Linux.
 *
 */

#include <string_view>

namespace coral66
{
enum class Architecture
{
    x86_64,
    Unknown,
};

enum class Platform
{
    Linux,
    Unknown,
};

enum class Environment
{
    GNU,
    Unknown,
};

class Triple
{
    Architecture m_architecture;
    Platform m_operatingSystem;
    Environment m_environment;

public:
    static Triple fromString(std::string_view string);

    Triple(Architecture architecture, Platform operatingSystem, Environment environment);

    [[nodiscard]] Architecture getArchitecture() const
    {
        return m_architecture;
    }

    [[nodiscard]] Platform getPlatform() const
    {
        return m_operatingSystem;
    }

    [[nodiscard]] Environment getEnvironment() const
    {
        return m_environment;
    }

    static Triple defaultTarget();

    static Triple native();

    bool operator==(const Triple& rhs) const
    {
        return m_architecture == rhs.m_architecture && m_operatingSystem == rhs.m_operatingSystem
               && m_environment == rhs.m_environment;
    }

    bool operator!=(const Triple& rhs) const
    {
        return !(rhs == *this);
    }
};

} // namespace coral66
