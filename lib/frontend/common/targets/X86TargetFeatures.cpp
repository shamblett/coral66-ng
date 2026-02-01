/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */
 
#include "X86TargetFeatures.hpp"

bool coral66::X86TargetFeatures::hasFeature(coral66::TargetFeatures::Features const features) const
{
    switch (features)
    {
        default: return false;
        case TargetFeatures::IsX86: return true;
#define HANDLE_VALUE(x) \
    case x: return m_## x;
//
#include "X86Features.def"
    }
}

bool coral66::X86TargetFeatures::setFeature(coral66::TargetFeatures::Features const features, bool value)
{
    switch (features)
    {
        default: return false;
#define HANDLE_VALUE(x) \
    case x: m_## x = value; return true;
//
#include "X86Features.def"
    }
}
