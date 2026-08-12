/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "x86_target_features.hpp"

bool coral66::x86_target_features::hasFeature(Features const features) const
{
    switch (features)
    {
        default: return false;
        case IsX86: return true;
#define HANDLE_VALUE(x) \
    case x: return m_##x;
//
#include "x86_features.def"
    }
}

bool coral66::x86_target_features::setFeature(Features const features, const bool value)
{
    switch (features)
    {
        default: return false;
#define HANDLE_VALUE(x) \
    case x: m_##x = value; return true;
//
#include "x86_features.def"
    }
}
