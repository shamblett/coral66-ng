/*
 * Project: coral66-ng-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

#include "lib/support/triple.hpp"

namespace coral66::Tests
{

const inline auto x64linux = Triple(Architecture::x86_64, Platform::Linux, Environment::GNU);

} // namespace coral66::Tests
