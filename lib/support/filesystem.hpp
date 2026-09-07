/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

#include <filesystem>
#include <fstream>

/**
 *
 * This wrapper namespace (coral66::fs) was originally introduced
 * to polyfill std::filesystem using the `ghc::filesystem` third-party library
 * on older C++ standard versions/compilers that lacked full C++17 <filesystem> support.
 * Since the project now utilizes C++23 features, std::filesystem is natively supported.
 * The wrapper is maintained strictly for API compatibility to avoid refactoring
 * `coral66::fs` namespace qualifiers scattered across the codebase.
 *
 */

namespace coral66::fs
{
using namespace std::filesystem;

using ifstream = std::ifstream;
using ofstream = std::ofstream;
using fstream = std::fstream;
} // namespace coral66::fs
