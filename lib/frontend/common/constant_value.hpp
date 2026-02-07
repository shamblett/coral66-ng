/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/APSInt.h>

#include <string>
#include <variant>

#include "language_options.hpp"

namespace coral66::Semantics
{
class Type;
class ProgramInterface;
class Issue;

class constant_value final
{
public:
    using ValueType = std::variant<llvm::APSInt, llvm::APFloat, std::string>;

private:
    ValueType m_value;

public:
    constant_value() = default;

    /*implicit*/ constant_value(ValueType value) : m_value(std::move(value)) {}

    constant_value(llvm::APSInt integer);
    constant_value(llvm::APFloat floating);
    constant_value(std::string string);

    constant_value castTo(const Type& type, const ProgramInterface* program, const LanguageOptions& options,
                      Issue* issue = nullptr) const;

    // Add accessors if needed by other parts of the system
    const ValueType& getValue() const { return m_value; }
};

} // namespace coral66::Semantics
