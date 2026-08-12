/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "language_options.hpp"

bool coral66::DefaultTargetFeatures::hasFeature(Features) const
{
    return false;
}

bool coral66::DefaultTargetFeatures::setFeature(Features, bool)
{
    return false;
}
