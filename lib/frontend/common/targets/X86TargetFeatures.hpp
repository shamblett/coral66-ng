/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once
/**
 * Target feature support
 *
 */

#include "frontend/compiler/LanguageOptions.hpp"

namespace coral66
{
 // ReSharper disable once CppClassNeedsConstructorBecauseOfUninitializedMember
class X86TargetFeatures final : public TargetFeatures
{
#define HANDLE_VALUE(x) bool m_## x;
#include "X86Features.def"
public:
    bool hasFeature(Features features) const override;

    bool setFeature(Features features, bool value) override;
};
} // namespace coral66
