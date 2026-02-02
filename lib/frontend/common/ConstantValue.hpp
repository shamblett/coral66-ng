/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

#include <llvm/ADT/APSInt.h>
#include <llvm/ADT/APFloat.h>
#include <variant>
#include <string>
#include <optional>

#include "DiagnosticUtilities.hpp"
#include "LanguageOptions.hpp"

namespace coral66::Semantics
{
class Type;
class ProgramInterface;
class Issue;

class ConstantValue final
{
public:
    using ValueType = std::variant<llvm::APSInt, llvm::APFloat, std::string>;

private:
    ValueType m_value;

public:
    ConstantValue() = default;

    /*implicit*/ ConstantValue(ValueType value) : m_value(std::move(value)) {}

    ConstantValue(llvm::APSInt integer);
    ConstantValue(llvm::APFloat floating);
    ConstantValue(std::string string);

    ConstantValue castTo(const Type& type, const ProgramInterface* program, const LanguageOptions& options,
                      Issue* issue = nullptr) const;

    // Add accessors if needed by other parts of the system
    const ValueType& getValue() const { return m_value; }
};

} // namespace coral66::Semantics
