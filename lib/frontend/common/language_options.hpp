/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

/**
 * Language definitions and options
 *
 */

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_set>

#include "lib/support/triple.hpp"

namespace coral66
{
class TargetFeatures
{
public:
    virtual ~TargetFeatures() = default;

    enum Features
    {
        IsX86,
    // x86 features
#define HANDLE_VALUE(x) x,
#include "lib/frontend/common/targets/x86_features.def"
    };

    virtual bool hasFeature(Features features) const = 0;

    virtual bool setFeature(Features features, bool value) = 0;
};

class DefaultTargetFeatures final : public TargetFeatures
{
public:
    bool hasFeature(Features features) const override;

    bool setFeature(Features features, bool value) override;
};

struct LanguageOptions
{
    std::shared_ptr<TargetFeatures> targetFeatures = std::make_shared<DefaultTargetFeatures>();
    std::unordered_set<std::string_view> enabledWarnings{};
    bool freeStanding{};
    enum class Extension
    {
        None,
    } extension{Extension::None};
    enum class BuiltInVaList
    {
        x86_64ABI
    } vaListKind{};
};

} // namespace coral66
