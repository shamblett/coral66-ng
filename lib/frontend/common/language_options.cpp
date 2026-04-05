/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "language_options.hpp"

#include <limits>
#include <numeric>

#include "diagnostic.hpp"
#include "lib/support/utilities.hpp"
#include "targets/x86_target_features.hpp"

bool coral66::DefaultTargetFeatures::hasFeature(Features) const
{
    return false;
}

bool coral66::DefaultTargetFeatures::setFeature(Features, bool)
{
    return false;
}
