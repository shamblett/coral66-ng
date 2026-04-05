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

#include "lib/frontend//common/language_options.hpp"

namespace coral66
{
// ReSharper disable once CppClassNeedsConstructorBecauseOfUninitializedMember
class x86_target_features final : public TargetFeatures
{
#define HANDLE_VALUE(x) bool m_##x;
#include "x86_features.def"
public:
    [[nodiscard]] bool hasFeature(Features features) const override;

    bool setFeature(Features features, bool value) override;
};
} // namespace coral66
