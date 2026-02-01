/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "ConstantValue.hpp"
#include "SemanticsCommon.hpp"

namespace coral66::Semantics// for std::tie
{

ConstantValue::ConstantValue(llvm::APSInt integer) : m_value(std::move(integer)) {}
ConstantValue::ConstantValue(llvm::APFloat floating) : m_value(std::move(floating)) {}
ConstantValue::ConstantValue(std::string string) : m_value(std::move(string)) {}

ConstantValue ConstantValue::castTo(const Type& type, const ProgramInterface*, const LanguageOptions&, Issue*) const
{
    if (auto* prim = type.tryAs<PrimitiveType>())
    {
        if (prim->getKind() == PrimitiveType::Integer)
        {
             if (std::holds_alternative<llvm::APFloat>(m_value))
             {
                 bool ignored;
                 llvm::APSInt res(64, true); // Assume 64-bit signed integer
                 std::get<llvm::APFloat>(m_value).convertToInteger(res, llvm::APFloat::rmTowardZero, &ignored);
                 return ConstantValue(res);
             }
        }
        else if (prim->getKind() == PrimitiveType::Floating)
        {
             if (std::holds_alternative<llvm::APSInt>(m_value))
             {
                 llvm::APFloat res(llvm::APFloat::IEEEdouble());
                 res.convertFromAPInt(std::get<llvm::APSInt>(m_value), std::get<llvm::APSInt>(m_value).isSigned(), llvm::APFloat::rmNearestTiesToEven);
                 return ConstantValue(res);
             }
        }
    }
    return *this;
}

} // namespace coral66::Semantics
