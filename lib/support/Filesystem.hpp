/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once


#include <filesystem>
#include <fstream>

namespace coral66::fs
{
using namespace std::filesystem;

using ifstream = std::ifstream;
using ofstream = std::ofstream;
using fstream = std::fstream;
} // namespace coral66::fs

