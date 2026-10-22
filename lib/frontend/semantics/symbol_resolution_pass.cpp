
/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 02/05/2026
 */

#include "symbol_resolution_pass.hpp"

#include <functional>
#include <iostream>
#include <string>

#include "cmi_processor.hpp"
#include "control_flow_pass.hpp"
#include "lib/frontend/common/diagnostic_collection.hpp"
#include "lib/frontend/common/messages/errors.hpp"
#include "semantic_support.hpp"
#include "semantic_utilities.hpp"

namespace coral66::semantics
{

template <class T, class... Args>
static IntrVarPtr<T> makeIntrusiveVariant(Args&&... args)
{
    return IntrVarPtr<T>(new T(std::forward<Args>(args)...));
}

SymbolResolutionPass::SymbolResolutionPass(SemanticContext& context) : m_context(context) {}

void SymbolResolutionPass::run(const syntax::Program& program)
{
    visit(program);
}

void SymbolResolutionPass::visit(const syntax::Program& node)
{
    m_context.symbolTable.enterScope();

    for (const auto& comm : node.getCommunicators())
    {
        if (auto* commonCommunicator = dynamic_cast<const syntax::CommonCommunicator*>(comm.get()))
        {
            const size_t usablesBefore = m_context.ownedUsables.size();
            for (const auto& item : commonCommunicator->getItems())
            {
                visit(*item);
            }
            for (size_t i = usablesBefore; i < m_context.ownedUsables.size(); ++i)
            {
                auto* usable = m_context.ownedUsables[i].get();
                usable->match([&](VariableDeclaration& decl) { decl.setCommunicatorType(CommunicatorType::Common); },
                              [&](ProcedureDeclaration& decl) { decl.setCommunicatorType(CommunicatorType::Common); },
                              [&](ArrayDeclaration& decl) { decl.setCommunicatorType(CommunicatorType::Common); },
                              [&](TableDeclaration& decl) { decl.setCommunicatorType(CommunicatorType::Common); },
                              [&](OverlayDeclaration& decl) { decl.setCommunicatorType(CommunicatorType::Common); },
                              [&](SwitchDeclaration& decl) { decl.setCommunicatorType(CommunicatorType::Common); },
                              [&](LabelDeclaration& decl) { decl.setCommunicatorType(CommunicatorType::Common); },
                              [&](TableElementDeclaration& decl)
                              { decl.setCommunicatorType(CommunicatorType::Common); });
                m_context.exportedSymbols.push_back(usable);
            }
        }
        else if (auto* extCommunicator = dynamic_cast<const syntax::ExternalCommunicator*>(comm.get()))
        {
            const size_t usablesBefore = m_context.ownedUsables.size();
            for (const auto& item : extCommunicator->getItems())
            {
                visit(*item);
            }
            for (size_t i = usablesBefore; i < m_context.ownedUsables.size(); ++i)
            {
                auto* usable = m_context.ownedUsables[i].get();
                usable->match([&](VariableDeclaration& decl) { decl.setCommunicatorType(CommunicatorType::External); },
                              [&](ProcedureDeclaration& decl) { decl.setCommunicatorType(CommunicatorType::External); },
                              [&](ArrayDeclaration& decl) { decl.setCommunicatorType(CommunicatorType::External); },
                              [&](TableDeclaration& decl) { decl.setCommunicatorType(CommunicatorType::External); },
                              [&](OverlayDeclaration& decl) { decl.setCommunicatorType(CommunicatorType::External); },
                              [&](SwitchDeclaration& decl) { decl.setCommunicatorType(CommunicatorType::External); },
                              [&](LabelDeclaration& decl) { decl.setCommunicatorType(CommunicatorType::External); },
                              [&](TableElementDeclaration& decl)
                              { decl.setCommunicatorType(CommunicatorType::External); });
            }
        }
        else if (auto* absoluteCommunicator = dynamic_cast<const syntax::AbsoluteCommunicator*>(comm.get()))
        {
            const size_t usablesBefore = m_context.ownedUsables.size();
            for (const auto& item : absoluteCommunicator->getItems())
            {
                visit(*item);
            }
            for (size_t i = usablesBefore; i < m_context.ownedUsables.size(); ++i)
            {
                auto* usable = m_context.ownedUsables[i].get();
                Declaration* decl = nullptr;
                if (auto* var = usable->tryAs<VariableDeclaration>())
                    decl = var;
                else if (auto* arr = usable->tryAs<ArrayDeclaration>())
                    decl = arr;
                else if (auto* tab = usable->tryAs<TableDeclaration>())
                    decl = tab;

                if (decl)
                {
                    if (auto it = absoluteCommunicator->getAddresses().find(decl->getNameToken());
                        it != absoluteCommunicator->getAddresses().end())
                    {
                        if (auto val = evaluateConstantInteger(*it->second))
                        {
                            decl->setAbsoluteAddress(*val);
                            decl->setCommunicatorType(CommunicatorType::Absolute);
                        }
                        else
                        {
                            m_context.log(errors::semantics::EXPECTED_CONSTANT_EXPRESSION.args(
                                *it->second->begin(), m_context.source, *it->second->begin()));
                            DiagnosticCollector::addDiagnostic(errors::semantics::EXPECTED_CONSTANT_EXPRESSION);
                        }
                    }
                }
            }
        }
        else if (auto* libCommunicator = dynamic_cast<const syntax::LibraryCommunicator*>(comm.get()))
        {
            for (const auto& itemToken : libCommunicator->getItems())
            {
                const auto& libName = std::get<std::string>(itemToken->getValue());
                std::filesystem::path cmiPath = m_context.libraryPath;
                cmiPath /= libName + ".cmi";

                if (std::vector<std::unique_ptr<Declaration>> importedSymbols;
                    !CMIProcessor::parseCMIFile(cmiPath, importedSymbols))
                {
                    m_context.log(errors::semantics::FAILED_TO_READ_CMI_FILE_N.args(*itemToken, m_context.source,
                                                                                    *itemToken, cmiPath.string()));
                    DiagnosticCollector::addDiagnostic(errors::semantics::FAILED_TO_READ_CMI_FILE_N);
                }
                else
                {
                    for (auto& decl : importedSymbols)
                    {
                        const auto name = decl->getName(); // Returns string_view
                        decl->setScopeDepth(m_context.symbolTable.scopeCount());

                        // Convert to IntrusiveVariantValue
                        IntrVarPtr<Usable> usableDecl(decl.release());

                        if (m_context.symbolTable.lookupInCurrentScope(name))
                        {
                            m_context.log(errors::semantics::REDEFINITION_OF_SYMBOL_N.args(*itemToken, m_context.source,
                                                                                           *itemToken));
                            DiagnosticCollector::addDiagnostic(errors::semantics::REDEFINITION_OF_SYMBOL_N);
                        }
                        else
                        {
                            m_context.symbolTable.insert(name, usableDecl.get());
                        }
                        m_context.ownedUsables.push_back(std::move(usableDecl));
                    }
                }
            }
        }
    }

    if (node.hasBlock())
    {
        visit(node.getBlock());
    }
    m_context.symbolTable.exitScope();
}

void SymbolResolutionPass::visit(const syntax::Block& node)
{
    m_context.symbolTable.enterScope();

    // Pre-scan all statements for labels to ensure correct scope and forward-referencing
    std::function<void(const syntax::Statement&)> preScanLabels = [&](const syntax::Statement& stmt)
    {
        for (const auto& label : stmt.getLabels())
        {
            auto declaration = makeIntrusiveVariant<LabelDeclaration>(label);
            const auto& name = std::get<std::string>(label->getValue());
            if (m_context.symbolTable.lookupInCurrentScope(name))
            {
                m_context.log(errors::semantics::REDEFINITION_OF_SYMBOL_N.args(*label, m_context.source, *label));
                DiagnosticCollector::addDiagnostic(errors::semantics::REDEFINITION_OF_SYMBOL_N);
            }
            else
            {
                if (const Usable* existing = m_context.symbolTable.lookup(name))
                {
                    existing->match(
                        [&](const auto& existingDecl)
                        {
                            declaration->setCommunicatorType(existingDecl.getCommunicatorType());
                            if (existingDecl.getAbsoluteAddress().has_value())
                                declaration->setAbsoluteAddress(existingDecl.getAbsoluteAddress().value());
                        });
                }
                declaration->setScopeDepth(m_context.symbolTable.scopeCount());
                m_context.symbolTable.insert(name, declaration.get());
            }
            m_context.ownedUsables.push_back(std::move(declaration));
        }

        if (auto* compStmt = dynamic_cast<const syntax::CompoundStatement*>(&stmt))
        {
            for (const auto& innerStmt : compStmt->getStatements())
                preScanLabels(*innerStmt);
        }
        else if (auto* ifStmt = dynamic_cast<const syntax::IfStatement*>(&stmt))
        {
            preScanLabels(ifStmt->getThenBranch());
            if (ifStmt->getElseBranch())
                preScanLabels(*ifStmt->getElseBranch());
        }
        else if (auto* forStmt = dynamic_cast<const syntax::ForStatement*>(&stmt))
        {
            preScanLabels(forStmt->getBody());
        }
        // BlockStatements create a new scope, so we do NOT recurse into them during pre-scan.
    };

    for (const auto& stmt : node.getStatements())
    {
        preScanLabels(*stmt);
    }

    for (const auto& declaration : node.getDeclarations())
    {
        visit(*declaration);
    }

    for (const auto& stmt : node.getStatements())
    {
        visit(*stmt);
    }

    m_context.symbolTable.exitScope();
}

void SymbolResolutionPass::visit(const syntax::Statement& node)
{
    if (auto* blockStmt = dynamic_cast<const syntax::BlockStatement*>(&node))
        visit(*blockStmt);
    else if (auto* compStmt = dynamic_cast<const syntax::CompoundStatement*>(&node))
        visit(*compStmt);
    else if (auto* ifStmt = dynamic_cast<const syntax::IfStatement*>(&node))
        visit(*ifStmt);
    else if (auto* forStmt = dynamic_cast<const syntax::ForStatement*>(&node))
        visit(*forStmt);
    else if (auto* assignStmt = dynamic_cast<const syntax::AssignmentStatement*>(&node))
        visit(*assignStmt);
    else if (auto* gotoStmt = dynamic_cast<const syntax::GotoStatement*>(&node))
        visit(*gotoStmt);
    else if (auto* answerStmt = dynamic_cast<const syntax::AnswerStatement*>(&node))
        visit(*answerStmt);
    else if (auto* codeStmt = dynamic_cast<const syntax::CodeStatement*>(&node))
        visit(*codeStmt);
    else if (auto* callStmt = dynamic_cast<const syntax::ProcedureCallStatement*>(&node))
        visit(*callStmt);
}

void SymbolResolutionPass::visit(const syntax::BlockStatement& node)
{
    if (node.hasBlock())
        visit(node.getBlock());
}

void SymbolResolutionPass::visit(const syntax::CompoundStatement& node)
{
    for (const auto& stmt : node.getStatements())
        visit(*stmt);
}

void SymbolResolutionPass::visit(const syntax::IfStatement& node)
{
    visit(node.getCondition());
    visit(node.getThenBranch());
    if (node.getElseBranch())
        visit(*node.getElseBranch());
}

void SymbolResolutionPass::visit(const syntax::ForStatement& node)
{
    visit(node.getVariable());
    for (const auto& [expression, step, until, whileCondition] : node.getElements())
    {
        visit(*expression);
        if (step)
            visit(*step);
        if (until)
            visit(*until);
        if (whileCondition)
            visit(*whileCondition);
    }
    visit(node.getBody());
}

void SymbolResolutionPass::visit(const syntax::AssignmentStatement& node)
{
    visit(node.getVariable());
    visit(node.getExpression());
}

void SymbolResolutionPass::visit(const syntax::GotoStatement& node)
{
    const auto& labelToken = node.getLabel();
    const auto& labelName = std::get<std::string>(labelToken->getValue());

    if (const Usable* usable = m_context.symbolTable.lookup(labelName))
    {
        bool isLabelOrSwitch = usable->is<LabelDeclaration>() || usable->is<SwitchDeclaration>();
        if (!isLabelOrSwitch)
        {
            if (usable->is<VariableDeclaration>())
            {
                const auto& varDecl = usable->as<VariableDeclaration>();
                isLabelOrSwitch = varDecl.getType().is<LabelType>() || varDecl.getType().is<SwitchType>();
            }
        }

        if (!isLabelOrSwitch)
        {
            m_context.log(errors::semantics::LABEL_NOT_FOUND_N.args(*labelToken, m_context.source, *labelToken));
            DiagnosticCollector::addDiagnostic(errors::semantics::LABEL_NOT_FOUND_N);
        }
        else
        {
            m_context.resolvedIdentifiers[&node] = usable;
        }
    }
    else
    {
        m_context.log(errors::semantics::UNDECLARED_IDENTIFIER_N.args(*labelToken, m_context.source, *labelToken));
        DiagnosticCollector::addDiagnostic(errors::semantics::UNDECLARED_IDENTIFIER_N);
    }

    if (node.getIndex())
        visit(*node.getIndex());
}

void SymbolResolutionPass::visit(const syntax::AnswerStatement& node)
{
    visit(node.getExpression());
}

void SymbolResolutionPass::visit(const syntax::ProcedureCallStatement& node)
{
    visit(node.getExpression());
}

// ReSharper disable once CppMemberFunctionMayBeStatic
IntrusiveVariantValue<Type> SymbolResolutionPass::createTypeFromToken( // NOLINT(*-convert-member-functions-to-static)
    const std::optional<lexer::CTokenIterator> typeToken, const syntax::Expression* fixedTotalBits,
    const syntax::Expression* fixedFractionBits) // NOLINT(*-convert-member-functions-to-static)
{
    if (!typeToken)
        return {UntypedType{}};

    const auto type = typeToken.value()->getTokenType();
    if (type == lexer::TokenType::FloatingKeyword)
        return {PrimitiveType{PrimitiveType::Floating}};
    if (type == lexer::TokenType::PointerKeyword)
        return {PrimitiveType{PrimitiveType::Pointer}};
    if (type == lexer::TokenType::FixedKeyword)
    {
        uint32_t total = 16;
        uint32_t frac = 8;
        if (fixedTotalBits)
        {
            if (const auto tVal = evaluateConstantInteger(*fixedTotalBits); tVal && *tVal > 0)
                total = static_cast<uint32_t>(*tVal);
        }
        if (fixedFractionBits)
        {
            if (const auto fVal = evaluateConstantInteger(*fixedFractionBits); fVal && *fVal >= 0)
                frac = static_cast<uint32_t>(*fVal);
        }
        return {FixedType{static_cast<uint8_t>(total), static_cast<uint8_t>(frac)}};
    }
    if (type == lexer::TokenType::BitsKeyword || type == lexer::TokenType::BitKeyword)
        return {PrimitiveType{PrimitiveType::Bits}};

    return {PrimitiveType{PrimitiveType::Integer}};
}

void SymbolResolutionPass::visit(const syntax::Declaration& node)
{
    if (auto* variableDeclaration = dynamic_cast<const syntax::VariableDeclaration*>(&node))
        visit(*variableDeclaration);
    else if (auto* procedureDeclaration = dynamic_cast<const syntax::ProcedureDeclaration*>(&node))
        visit(*procedureDeclaration);
    else if (auto* procedureSpecification = dynamic_cast<const syntax::ProcedureSpecification*>(&node))
        visit(*procedureSpecification);
    else if (auto* placeSpecification = dynamic_cast<const syntax::PlaceSpecification*>(&node))
        visit(*placeSpecification);
    else if (auto* arrayDeclaration = dynamic_cast<const syntax::ArrayDeclaration*>(&node))
        visit(*arrayDeclaration);
    else if (auto* tableDeclaration = dynamic_cast<const syntax::TableDeclaration*>(&node))
        visit(*tableDeclaration);
    else if (auto* overlayDeclaration = dynamic_cast<const syntax::OverlayDeclaration*>(&node))
        visit(*overlayDeclaration);
    else if (auto* parameterDeclaration = dynamic_cast<const syntax::ParameterDeclaration*>(&node))
        visit(*parameterDeclaration);
    else if (auto* switchDeclaration = dynamic_cast<const syntax::SwitchDeclaration*>(&node))
        visit(*switchDeclaration);
    else if (auto* tableElementDeclaration = dynamic_cast<const syntax::TableElementDeclaration*>(&node))
        visit(*tableElementDeclaration);
}

void SymbolResolutionPass::visit(const syntax::VariableDeclaration& node)
{
    auto type = createTypeFromToken(node.getType(), node.getFixedTotalBits(), node.getFixedFractionBits());
    std::size_t identifierIndex = 0;
    for (auto id : node.getIdentifiers())
    {
        auto typeCopy = IntrusiveVariantValue(PrimitiveType{PrimitiveType::Integer});
        if (type->is<PrimitiveType>())
            typeCopy = IntrusiveVariantValue(PrimitiveType{type->as<PrimitiveType>().getKind()});
        else if (type->is<FixedType>())
            typeCopy = IntrusiveVariantValue(
                FixedType{type->as<FixedType>().getTotalBits(), type->as<FixedType>().getFractionalBits()});

        auto declaration = makeIntrusiveVariant<VariableDeclaration>(id, std::move(typeCopy));
        declaration->setAstNode(&node);
        declaration->setPresetIndex(identifierIndex++);
        const auto& name = std::get<std::string>(id->getValue());

        if (m_context.symbolTable.lookupInCurrentScope(name))
        {
            m_context.log(errors::semantics::REDEFINITION_OF_SYMBOL_N.args(*id, m_context.source, *id));
            DiagnosticCollector::addDiagnostic(errors::semantics::REDEFINITION_OF_SYMBOL_N);
        }
        else
        {
            if (const Usable* existing = m_context.symbolTable.lookup(name))
            {
                existing->match(
                    [&](const auto& existingDecl)
                    {
                        declaration->setCommunicatorType(existingDecl.getCommunicatorType());
                        if (existingDecl.getAbsoluteAddress().has_value())
                            declaration->setAbsoluteAddress(existingDecl.getAbsoluteAddress().value());
                    });
            }
            declaration->setScopeDepth(m_context.symbolTable.scopeCount());
            m_context.symbolTable.insert(name, declaration.get());
        }
        m_context.ownedUsables.push_back(std::move(declaration));
    }
}

void SymbolResolutionPass::visit(const syntax::ArrayDeclaration& node)
{
    auto baseTypeVal = createTypeFromToken(node.getType(), node.getFixedTotalBits(), node.getFixedFractionBits());
    m_context.ownedTypes.push_back(std::move(baseTypeVal));
    const Type* baseTypePtr = m_context.ownedTypes.back().data();

    std::vector<std::uint64_t> dims;
    for (const auto& [fst, snd] : node.getDimensions())
    {
        auto lower = evaluateConstantInteger(*fst);
        if (auto upper = evaluateConstantInteger(*snd); lower && upper)
        {
            if (*lower <= *upper)
            {
                dims.push_back(static_cast<std::uint64_t>(*upper - *lower + 1));
            }
            else
            {
                dims.push_back(0); // Handled by Control Flow validation
            }
        }
        else
        {
            dims.push_back(0);
        }
    }
    std::size_t arraySize = 1;
    for (const auto dim : dims)
    {
        if (dim > 0)
            arraySize *= dim;
    }

    std::size_t elementOffset = 0;
    for (auto id : node.getIdentifiers())
    {
        auto typeCopy = IntrusiveVariantValue(ArrayType{baseTypePtr, dims});

        auto declaration = makeIntrusiveVariant<ArrayDeclaration>(id, std::move(typeCopy));
        declaration->setAstNode(&node);
        declaration->setPresetIndex(elementOffset);
        elementOffset += arraySize;
        const auto& name = std::get<std::string>(id->getValue());

        if (m_context.symbolTable.lookupInCurrentScope(name))
        {
            m_context.log(errors::semantics::REDEFINITION_OF_SYMBOL_N.args(*id, m_context.source, *id));
            DiagnosticCollector::addDiagnostic(errors::semantics::REDEFINITION_OF_SYMBOL_N);
        }
        else
        {
            if (const Usable* existing = m_context.symbolTable.lookup(name))
            {
                existing->match(
                    [&](const auto& existingDecl)
                    {
                        declaration->setCommunicatorType(existingDecl.getCommunicatorType());
                        if (existingDecl.getAbsoluteAddress().has_value())
                            declaration->setAbsoluteAddress(existingDecl.getAbsoluteAddress().value());
                    });
            }
            declaration->setScopeDepth(m_context.symbolTable.scopeCount());
            m_context.symbolTable.insert(name, declaration.get());
        }
        m_context.ownedUsables.push_back(std::move(declaration));
    }

    // Visit dimension expressions
    for (const auto& [fst, snd] : node.getDimensions())
    {
        if (fst)
            visit(*fst);
        if (snd)
            visit(*snd);
    }
}

void SymbolResolutionPass::visit(const syntax::TableDeclaration& node)
{
    if (node.getWidth())
        visit(*node.getWidth());
    if (node.getLength())
        visit(*node.getLength());

    auto id = node.getIdentifier();

    std::uint64_t widthVal = 0;
    std::uint64_t lengthVal = 0;
    if (node.getWidth())
    {
        if (const auto w = evaluateConstantInteger(*node.getWidth()))
            widthVal = static_cast<std::uint64_t>(*w);
    }
    if (node.getLength())
    {
        if (const auto l = evaluateConstantInteger(*node.getLength()))
            lengthVal = static_cast<std::uint64_t>(*l);
    }

    m_context.ownedTableInfos.push_back(TableInfo{
        .name = std::get<std::string>(id->getValue()), .id = 0, .type = TableDefinition{{}, widthVal, lengthVal}});

    auto typeCopy = IntrusiveVariantValue(TableType{m_context.ownedTableInfos.back()});
    auto declaration = makeIntrusiveVariant<TableDeclaration>(id, std::move(typeCopy));
    declaration->setAstNode(&node);

    auto* prevTableDeclaration = m_currentTableDeclaration;
    m_currentTableDeclaration = declaration.get();

    const auto& name = std::get<std::string>(id->getValue());

    if (m_context.symbolTable.lookupInCurrentScope(name))
    {
        m_context.log(errors::semantics::REDEFINITION_OF_SYMBOL_N.args(*id, m_context.source, *id));
        DiagnosticCollector::addDiagnostic(errors::semantics::REDEFINITION_OF_SYMBOL_N);
    }
    else
    {
        if (const Usable* existing = m_context.symbolTable.lookup(name))
        {
            existing->match(
                [&](const auto& existingDecl)
                {
                    declaration->setCommunicatorType(existingDecl.getCommunicatorType());
                    if (existingDecl.getAbsoluteAddress().has_value())
                        declaration->setAbsoluteAddress(existingDecl.getAbsoluteAddress().value());
                });
        }
        declaration->setScopeDepth(m_context.symbolTable.scopeCount());
        m_context.symbolTable.insert(name, declaration.get());
    }
    m_context.ownedUsables.push_back(std::move(declaration));

    for (const auto& element : node.getElements())
    {
        visit(*element);
    }

    m_currentTableDeclaration = prevTableDeclaration;
}

void SymbolResolutionPass::visit(const syntax::TableElementDeclaration& node)
{
    auto type = createTypeFromToken(node.getType(), node.getFixedTotalBits(), node.getFixedFractionBits());
    auto typeCopy = IntrusiveVariantValue(PrimitiveType{PrimitiveType::Integer}); // Default for now
    if (type->is<PrimitiveType>())
        typeCopy = IntrusiveVariantValue(PrimitiveType{type->as<PrimitiveType>().getKind()});
    else if (type->is<FixedType>())
        typeCopy = IntrusiveVariantValue(
            FixedType{type->as<FixedType>().getTotalBits(), type->as<FixedType>().getFractionalBits()});
    else if (node.getFixedTotalBits())
    {
        uint32_t total = 16;
        if (const auto tVal = evaluateConstantInteger(*node.getFixedTotalBits()); tVal && *tVal > 0)
            total = static_cast<uint32_t>(*tVal);
        if (node.getFixedFractionBits())
        {
            uint32_t frac = 0;
            if (const auto fVal = evaluateConstantInteger(*node.getFixedFractionBits()); fVal && *fVal >= 0)
                frac = static_cast<uint32_t>(*fVal);
            typeCopy = IntrusiveVariantValue(FixedType{static_cast<uint8_t>(total), static_cast<uint8_t>(frac)});
        }
        else
        {
            // It's just (10) or UNSIGNED (10) which means part-word integer, not FIXED.
            typeCopy = IntrusiveVariantValue(PrimitiveType{PrimitiveType::Integer});
        }
    }

    std::optional<std::uint32_t> wordPos;
    if (node.getWordPosition())
    {
        if (auto* i = std::get_if<llvm::APSInt>(&node.getWordPosition().value()->getValue()))
            wordPos = static_cast<std::uint32_t>(i->getZExtValue());
    }

    std::optional<std::uint32_t> bitPos;
    if (node.getBitPosition())
    {
        if (auto* i = std::get_if<llvm::APSInt>(&node.getBitPosition().value()->getValue()))
            bitPos = static_cast<std::uint32_t>(i->getZExtValue());
    }

    auto id = node.getIdentifier();
    auto declaration = makeIntrusiveVariant<TableElementDeclaration>(id, std::move(typeCopy), node.isUnsigned(),
                                                                     wordPos, bitPos, m_currentTableDeclaration);
    declaration->setAstNode(&node);
    const auto& name = std::get<std::string>(id->getValue());

    if (m_context.symbolTable.lookupInCurrentScope(name))
    {
        m_context.log(errors::semantics::REDEFINITION_OF_SYMBOL_N.args(*id, m_context.source, *id));
        DiagnosticCollector::addDiagnostic(errors::semantics::REDEFINITION_OF_SYMBOL_N);
    }
    else
    {
        if (const Usable* existing = m_context.symbolTable.lookup(name))
        {
            existing->match(
                [&](const auto& existingDecl)
                {
                    declaration->setCommunicatorType(existingDecl.getCommunicatorType());
                    if (existingDecl.getAbsoluteAddress().has_value())
                        declaration->setAbsoluteAddress(existingDecl.getAbsoluteAddress().value());
                });
        }
        declaration->setScopeDepth(m_context.symbolTable.scopeCount());
        m_context.symbolTable.insert(name, declaration.get());
    }
    m_context.ownedUsables.push_back(std::move(declaration));
}

// ReSharper disable once CppMemberFunctionMayBeConst
void SymbolResolutionPass::visit(const syntax::SwitchDeclaration& node)
{
    auto id = node.getIdentifier();

    std::vector<const LabelStatement*> switchLabels;
    std::vector<const LabelDeclaration*> switchLabelDecls;

    for (const auto& labelToken : node.getLabels())
    {
        const auto& labelName = std::get<std::string>(labelToken->getValue());
        if (const Usable* usable = m_context.symbolTable.lookup(labelName); !usable)
        {
            m_context.log(errors::semantics::UNDECLARED_IDENTIFIER_N.args(*labelToken, m_context.source, *labelToken));
            DiagnosticCollector::addDiagnostic(errors::semantics::UNDECLARED_IDENTIFIER_N);
        }
        else if (!usable->is<LabelDeclaration>())
        {
            m_context.log(errors::semantics::LABEL_NOT_FOUND_N.args(*labelToken, m_context.source, *labelToken));
            DiagnosticCollector::addDiagnostic(errors::semantics::LABEL_NOT_FOUND_N);
        }
        else
        {
            switchLabels.push_back(nullptr);
            switchLabelDecls.push_back(&usable->as<LabelDeclaration>());
        }
    }

    auto declaration =
        makeIntrusiveVariant<SwitchDeclaration>(id, std::move(switchLabels), std::move(switchLabelDecls));
    declaration->setAstNode(&node);
    const auto& name = std::get<std::string>(id->getValue());

    if (m_context.symbolTable.lookupInCurrentScope(name))
    {
        m_context.log(errors::semantics::REDEFINITION_OF_SYMBOL_N.args(*id, m_context.source, *id));
        DiagnosticCollector::addDiagnostic(errors::semantics::REDEFINITION_OF_SYMBOL_N);
    }
    else
    {
        if (const Usable* existing = m_context.symbolTable.lookup(name))
        {
            existing->match(
                [&](const auto& existingDecl)
                {
                    declaration->setCommunicatorType(existingDecl.getCommunicatorType());
                    if (existingDecl.getAbsoluteAddress().has_value())
                        declaration->setAbsoluteAddress(existingDecl.getAbsoluteAddress().value());
                });
        }
        declaration->setScopeDepth(m_context.symbolTable.scopeCount());
        m_context.symbolTable.insert(name, declaration.get());
    }
    m_context.ownedUsables.push_back(std::move(declaration));
}

void SymbolResolutionPass::visit(const syntax::OverlayDeclaration& node)
{
    const auto baseToken = node.getBase();
    const auto& baseName = std::get<std::string>(baseToken->getValue());

    if (const Usable* usable = m_context.symbolTable.lookup(baseName); !usable)
    {
        m_context.log(errors::semantics::UNDECLARED_IDENTIFIER_N.args(*baseToken, m_context.source, *baseToken));
        DiagnosticCollector::addDiagnostic(errors::semantics::UNDECLARED_IDENTIFIER_N);
    }

    // Evaluate overlay target expressions
    visit(node.getOverlayDeclaration());
}

void SymbolResolutionPass::visit(const syntax::ProcedureDeclaration& node)
{
    auto id = node.getName();

    auto returnTypeVal = createTypeFromToken(node.getTypeToken());
    m_context.ownedTypes.push_back(std::move(returnTypeVal));
    const Type* returnTypePtr = node.getTypeToken() ? m_context.ownedTypes.back().data() : nullptr;

    std::vector<ProcedureType::Parameter> parameters;
    for (const auto& paramDeclarationNode : node.getParameters())
    {
        if (auto* paramNode = dynamic_cast<const syntax::ParameterDeclaration*>(paramDeclarationNode.get()))
        {
            auto baseTypeVal = createTypeFromToken(paramNode->getType(), paramNode->getFixedTotalBits(),
                                                   paramNode->getFixedFractionBits());

            IntrusiveVariantValue paramTypeVal{UntypedType{}};

            bool isValue = false;
            switch (paramNode->getMode())
            {
                case syntax::ParameterDeclaration::ParameterMode::Value:
                    if (baseTypeVal->is<PrimitiveType>())
                        paramTypeVal = IntrusiveVariantValue(
                            PrimitiveType{baseTypeVal->as<PrimitiveType>().getKind(), Type::Value});
                    else if (baseTypeVal->is<FixedType>())
                        paramTypeVal = IntrusiveVariantValue(FixedType{baseTypeVal->as<FixedType>().getTotalBits(),
                                                                       baseTypeVal->as<FixedType>().getFractionalBits(),
                                                                       Type::Value});
                    else
                        paramTypeVal = std::move(baseTypeVal);
                    isValue = true;
                    break;
                case syntax::ParameterDeclaration::ParameterMode::Location:
                    if (baseTypeVal->is<PrimitiveType>())
                        paramTypeVal = IntrusiveVariantValue(
                            PrimitiveType{baseTypeVal->as<PrimitiveType>().getKind(), Type::Location});
                    else if (baseTypeVal->is<FixedType>())
                        paramTypeVal = IntrusiveVariantValue(FixedType{baseTypeVal->as<FixedType>().getTotalBits(),
                                                                       baseTypeVal->as<FixedType>().getFractionalBits(),
                                                                       Type::Location});
                    else
                        paramTypeVal = std::move(baseTypeVal);
                    break;
                case syntax::ParameterDeclaration::ParameterMode::Array:
                    m_context.ownedTypes.push_back(std::move(baseTypeVal));
                    paramTypeVal = IntrusiveVariantValue(ArrayType{m_context.ownedTypes.back().data(), {}});
                    break;
                case syntax::ParameterDeclaration::ParameterMode::Label:
                    paramTypeVal = IntrusiveVariantValue(LabelType{});
                    break;
                case syntax::ParameterDeclaration::ParameterMode::Switch:
                    paramTypeVal = IntrusiveVariantValue(SwitchType{});
                    break;
                case syntax::ParameterDeclaration::ParameterMode::Procedure:
                {
                    const Type* procReturnType = nullptr;
                    if (paramNode->getType())
                    {
                        m_context.ownedTypes.push_back(std::move(baseTypeVal));
                        procReturnType = m_context.ownedTypes.back().data();
                    }
                    paramTypeVal = IntrusiveVariantValue(ProcedureType{procReturnType, {}});
                    break;
                }
                case syntax::ParameterDeclaration::ParameterMode::UntypedValue:
                    paramTypeVal = IntrusiveVariantValue(UntypedType{Type::Value});
                    isValue = true;
                    break;
                case syntax::ParameterDeclaration::ParameterMode::UntypedLocation:
                    paramTypeVal = IntrusiveVariantValue(UntypedType{Type::Location});
                    break;
            }

            m_context.ownedTypes.push_back(std::move(paramTypeVal));
            const Type* finalParamTypePtr = m_context.ownedTypes.back().data();

            for (const auto paramId : paramNode->getIdentifiers())
            {
                parameters.push_back({.type = finalParamTypePtr,
                                      .name = std::get<std::string>(paramId->getValue()),
                                      .isValue = isValue});
            }
        }
        else if (auto* varNode = dynamic_cast<const syntax::VariableDeclaration*>(paramDeclarationNode.get()))
        {
            auto baseTypeVal =
                createTypeFromToken(varNode->getType(), varNode->getFixedTotalBits(), varNode->getFixedFractionBits());

            IntrusiveVariantValue paramTypeVal{UntypedType{}};

            if (baseTypeVal->is<PrimitiveType>())
                paramTypeVal =
                    IntrusiveVariantValue(PrimitiveType{baseTypeVal->as<PrimitiveType>().getKind(), Type::Location});
            else if (baseTypeVal->is<FixedType>())
                paramTypeVal =
                    IntrusiveVariantValue(FixedType{baseTypeVal->as<FixedType>().getTotalBits(),
                                                    baseTypeVal->as<FixedType>().getFractionalBits(), Type::Location});
            else
                paramTypeVal = std::move(baseTypeVal);

            m_context.ownedTypes.push_back(std::move(paramTypeVal));
            const Type* finalParamTypePtr = m_context.ownedTypes.back().data();

            for (const auto paramId : varNode->getIdentifiers())
            {
                parameters.push_back(
                    {.type = finalParamTypePtr, .name = std::get<std::string>(paramId->getValue()), .isValue = false});
            }
        }
        else if (auto* arrayNode = dynamic_cast<const syntax::ArrayDeclaration*>(paramDeclarationNode.get()))
        {
            auto baseTypeVal = createTypeFromToken(arrayNode->getType());
            m_context.ownedTypes.push_back(std::move(baseTypeVal));
            const Type* baseTypePtr = m_context.ownedTypes.back().data();

            m_context.ownedTypes.emplace_back(ArrayType{baseTypePtr, {}});
            const Type* finalParamTypePtr = m_context.ownedTypes.back().data();

            for (const auto paramId : arrayNode->getIdentifiers())
            {
                parameters.push_back(
                    {.type = finalParamTypePtr, .name = std::get<std::string>(paramId->getValue()), .isValue = false});
            }
        }
        else if (auto* tableNode = dynamic_cast<const syntax::TableDeclaration*>(paramDeclarationNode.get()))
        {
            std::uint64_t widthVal = 0;
            std::uint64_t lengthVal = 0;
            if (tableNode->getWidth())
            {
                if (const auto w = evaluateConstantInteger(*tableNode->getWidth()))
                    widthVal = static_cast<std::uint64_t>(*w);
            }
            if (tableNode->getLength())
            {
                if (const auto l = evaluateConstantInteger(*tableNode->getLength()))
                    lengthVal = static_cast<std::uint64_t>(*l);
            }

            const auto paramId = tableNode->getIdentifier();
            m_context.ownedTableInfos.push_back(TableInfo{.name = std::get<std::string>(paramId->getValue()),
                                                          .id = 0,
                                                          .type = TableDefinition{{}, widthVal, lengthVal}});

            m_context.ownedTypes.emplace_back(TableType{m_context.ownedTableInfos.back(), Type::Location});
            const Type* finalParamTypePtr = m_context.ownedTypes.back().data();

            parameters.push_back(
                {.type = finalParamTypePtr, .name = std::get<std::string>(paramId->getValue()), .isValue = false});
        }
    }

    auto typeCopy = IntrusiveVariantValue(ProcedureType{returnTypePtr, std::move(parameters)});

    auto declaration = makeIntrusiveVariant<ProcedureDeclaration>(id, std::move(typeCopy));
    declaration->setAstNode(&node);
    const auto& name = std::get<std::string>(id->getValue());

    auto registerProcedure = [&]
    {
        if (m_context.symbolTable.lookupInCurrentScope(name))
        {
            m_context.log(errors::semantics::REDEFINITION_OF_SYMBOL_N.args(*id, m_context.source, *id));
            DiagnosticCollector::addDiagnostic(errors::semantics::REDEFINITION_OF_SYMBOL_N);
        }
        else
        {
            if (const Usable* existing = m_context.symbolTable.lookup(name))
            {
                existing->match(
                    [&](const auto& existingDecl)
                    {
                        declaration->setCommunicatorType(existingDecl.getCommunicatorType());
                        if (existingDecl.getAbsoluteAddress().has_value())
                            declaration->setAbsoluteAddress(existingDecl.getAbsoluteAddress().value());
                    });
            }
            declaration->setScopeDepth(m_context.symbolTable.scopeCount());
            m_context.symbolTable.insert(name, declaration.get());
        }
    };

    // Always register before entering scope so it resolves its own calls!
    registerProcedure();

    m_context.symbolTable.enterScope();
    for (const auto& param : node.getParameters())
    {
        visit(*param);
    }
    visit(node.getBody());
    m_context.symbolTable.exitScope();

    m_context.ownedUsables.push_back(std::move(declaration));
}

void SymbolResolutionPass::visit(const syntax::ProcedureSpecification& node)
{
    auto id = node.getName();

    auto returnTypeVal = createTypeFromToken(node.getType());
    m_context.ownedTypes.push_back(std::move(returnTypeVal));
    const Type* returnTypePtr = node.getType() ? m_context.ownedTypes.back().data() : nullptr;

    std::vector<ProcedureType::Parameter> parameters;
    for (const auto& paramDeclarationNode : node.getParameters())
    {
        if (auto* paramNode = dynamic_cast<const syntax::ParameterDeclaration*>(paramDeclarationNode.get()))
        {
            auto baseTypeVal = createTypeFromToken(paramNode->getType(), paramNode->getFixedTotalBits(),
                                                   paramNode->getFixedFractionBits());

            IntrusiveVariantValue paramTypeVal{UntypedType{}};

            bool isValue = false;
            switch (paramNode->getMode())
            {
                case syntax::ParameterDeclaration::ParameterMode::Value:
                    if (baseTypeVal->is<PrimitiveType>())
                        paramTypeVal = IntrusiveVariantValue(
                            PrimitiveType{baseTypeVal->as<PrimitiveType>().getKind(), Type::Value});
                    else if (baseTypeVal->is<FixedType>())
                        paramTypeVal = IntrusiveVariantValue(FixedType{baseTypeVal->as<FixedType>().getTotalBits(),
                                                                       baseTypeVal->as<FixedType>().getFractionalBits(),
                                                                       Type::Value});
                    else
                        paramTypeVal = std::move(baseTypeVal);
                    isValue = true;
                    break;
                case syntax::ParameterDeclaration::ParameterMode::Location:
                    if (baseTypeVal->is<PrimitiveType>())
                        paramTypeVal = IntrusiveVariantValue(
                            PrimitiveType{baseTypeVal->as<PrimitiveType>().getKind(), Type::Location});
                    else if (baseTypeVal->is<FixedType>())
                        paramTypeVal = IntrusiveVariantValue(FixedType{baseTypeVal->as<FixedType>().getTotalBits(),
                                                                       baseTypeVal->as<FixedType>().getFractionalBits(),
                                                                       Type::Location});
                    else
                        paramTypeVal = std::move(baseTypeVal);
                    break;
                case syntax::ParameterDeclaration::ParameterMode::Array:
                    m_context.ownedTypes.push_back(std::move(baseTypeVal));
                    paramTypeVal = IntrusiveVariantValue(ArrayType{m_context.ownedTypes.back().data(), {}});
                    break;
                case syntax::ParameterDeclaration::ParameterMode::UntypedValue:
                    paramTypeVal = std::move(baseTypeVal);
                    break;
                case syntax::ParameterDeclaration::ParameterMode::UntypedLocation:
                    paramTypeVal = IntrusiveVariantValue(UntypedType{});
                    break;
                case syntax::ParameterDeclaration::ParameterMode::Label:
                    paramTypeVal = IntrusiveVariantValue(LabelType{});
                    break;
                case syntax::ParameterDeclaration::ParameterMode::Switch:
                    paramTypeVal = IntrusiveVariantValue(SwitchType{});
                    break;
                case syntax::ParameterDeclaration::ParameterMode::Procedure:
                    paramTypeVal = IntrusiveVariantValue(ProcedureType{nullptr, {}});
                    break;
            }

            m_context.ownedTypes.push_back(std::move(paramTypeVal));
            parameters.push_back({.type = m_context.ownedTypes.back().data(), .name = "", .isValue = isValue});
        }
        else if (auto* varNode = dynamic_cast<const syntax::VariableDeclaration*>(paramDeclarationNode.get()))
        {
            auto baseTypeVal =
                createTypeFromToken(varNode->getType(), varNode->getFixedTotalBits(), varNode->getFixedFractionBits());

            IntrusiveVariantValue paramTypeVal{UntypedType{}};

            if (baseTypeVal->is<PrimitiveType>())
                paramTypeVal =
                    IntrusiveVariantValue(PrimitiveType{baseTypeVal->as<PrimitiveType>().getKind(), Type::Location});
            else if (baseTypeVal->is<FixedType>())
                paramTypeVal =
                    IntrusiveVariantValue(FixedType{baseTypeVal->as<FixedType>().getTotalBits(),
                                                    baseTypeVal->as<FixedType>().getFractionalBits(), Type::Location});
            else
                paramTypeVal = std::move(baseTypeVal);

            m_context.ownedTypes.push_back(std::move(paramTypeVal));

            for (size_t i = 0; i < varNode->getIdentifiers().size(); ++i)
                parameters.push_back({.type = m_context.ownedTypes.back().data(), .name = "", .isValue = false});
        }
        else if (auto* arrayNode = dynamic_cast<const syntax::ArrayDeclaration*>(paramDeclarationNode.get()))
        {
            auto baseTypeVal = createTypeFromToken(arrayNode->getType());
            m_context.ownedTypes.push_back(std::move(baseTypeVal));
            const Type* baseTypePtr = m_context.ownedTypes.back().data();

            m_context.ownedTypes.emplace_back(ArrayType{baseTypePtr, {}});
            const Type* finalParamTypePtr = m_context.ownedTypes.back().data();

            for (size_t i = 0; i < arrayNode->getIdentifiers().size(); ++i)
                parameters.push_back({.type = finalParamTypePtr, .name = "", .isValue = false});
        }
        else if (auto* tableNode = dynamic_cast<const syntax::TableDeclaration*>(paramDeclarationNode.get()))
        {
            std::uint64_t widthVal = 0;
            std::uint64_t lengthVal = 0;
            if (tableNode->getWidth())
            {
                if (const auto w = evaluateConstantInteger(*tableNode->getWidth()))
                    widthVal = static_cast<std::uint64_t>(*w);
            }
            if (tableNode->getLength())
            {
                if (const auto l = evaluateConstantInteger(*tableNode->getLength()))
                    lengthVal = static_cast<std::uint64_t>(*l);
            }

            const auto paramId = tableNode->getIdentifier();
            m_context.ownedTableInfos.push_back(TableInfo{.name = std::get<std::string>(paramId->getValue()),
                                                          .id = 0,
                                                          .type = TableDefinition{{}, widthVal, lengthVal}});

            m_context.ownedTypes.emplace_back(TableType{m_context.ownedTableInfos.back(), Type::Location});
            const Type* finalParamTypePtr = m_context.ownedTypes.back().data();

            parameters.push_back({.type = finalParamTypePtr, .name = "", .isValue = false});
        }
    }

    const auto type = ProcedureType{returnTypePtr, std::move(parameters)};
    auto typeCopy = IntrusiveVariantValue(type);

    auto declaration = makeIntrusiveVariant<ProcedureDeclaration>(id, std::move(typeCopy));
    declaration->setAstNode(&node);
    const auto& name = std::get<std::string>(id->getValue());

    if (m_context.symbolTable.lookupInCurrentScope(name))
    {
        m_context.log(errors::semantics::REDEFINITION_OF_SYMBOL_N.args(*id, m_context.source, *id));
        DiagnosticCollector::addDiagnostic(errors::semantics::REDEFINITION_OF_SYMBOL_N);
    }
    else
    {
        if (const Usable* existing = m_context.symbolTable.lookup(name))
        {
            existing->match(
                [&](const auto& existingDecl)
                {
                    declaration->setCommunicatorType(existingDecl.getCommunicatorType());
                    if (existingDecl.getAbsoluteAddress().has_value())
                        declaration->setAbsoluteAddress(existingDecl.getAbsoluteAddress().value());
                });
        }
        declaration->setScopeDepth(m_context.symbolTable.scopeCount());
        m_context.symbolTable.insert(name, declaration.get());
    }

    m_context.ownedUsables.push_back(std::move(declaration));
}

// ReSharper disable once CppMemberFunctionMayBeConst
void SymbolResolutionPass::visit(const syntax::PlaceSpecification& node)
{
    for (auto id : node.getIdentifiers())
    {
        auto declaration = makeIntrusiveVariant<LabelDeclaration>(id);
        declaration->setAstNode(&node);
        const auto& name = std::get<std::string>(id->getValue());

        if (m_context.symbolTable.lookupInCurrentScope(name))
        {
            m_context.log(errors::semantics::REDEFINITION_OF_SYMBOL_N.args(*id, m_context.source, *id));
            DiagnosticCollector::addDiagnostic(errors::semantics::REDEFINITION_OF_SYMBOL_N);
        }
        else
        {
            if (const Usable* existing = m_context.symbolTable.lookup(name))
            {
                existing->match(
                    [&](const auto& existingDecl)
                    {
                        declaration->setCommunicatorType(existingDecl.getCommunicatorType());
                        if (existingDecl.getAbsoluteAddress().has_value())
                            declaration->setAbsoluteAddress(existingDecl.getAbsoluteAddress().value());
                    });
            }
            declaration->setScopeDepth(m_context.symbolTable.scopeCount());
            m_context.symbolTable.insert(name, declaration.get());
        }
        m_context.ownedUsables.push_back(std::move(declaration));
    }
}

void SymbolResolutionPass::visit(const syntax::ParameterDeclaration& node)
{
    auto type = createTypeFromToken(node.getType(), node.getFixedTotalBits(), node.getFixedFractionBits());

    // We should allocate a base type if we need to point to it (e.g., for Array parameters)
    const Type* baseTypePtr = nullptr;
    if (node.getMode() == syntax::ParameterDeclaration::ParameterMode::Array)
    {
        m_context.ownedTypes.push_back(
            createTypeFromToken(node.getType(), node.getFixedTotalBits(), node.getFixedFractionBits()));
        baseTypePtr = m_context.ownedTypes.back().data();
    }

    for (auto id : node.getIdentifiers())
    {
        IntrusiveVariantValue typeCopy{PrimitiveType{PrimitiveType::Integer}};

        switch (node.getMode())
        {
            case syntax::ParameterDeclaration::ParameterMode::Value:
                if (type->is<PrimitiveType>())
                    typeCopy = IntrusiveVariantValue(PrimitiveType{type->as<PrimitiveType>().getKind(), Type::Value});
                else if (type->is<FixedType>())
                    typeCopy = IntrusiveVariantValue(FixedType{type->as<FixedType>().getTotalBits(),
                                                               type->as<FixedType>().getFractionalBits(), Type::Value});
                break;
            case syntax::ParameterDeclaration::ParameterMode::Location:
                if (type->is<PrimitiveType>())
                    typeCopy =
                        IntrusiveVariantValue(PrimitiveType{type->as<PrimitiveType>().getKind(), Type::Location});
                else if (type->is<FixedType>())
                    typeCopy =
                        IntrusiveVariantValue(FixedType{type->as<FixedType>().getTotalBits(),
                                                        type->as<FixedType>().getFractionalBits(), Type::Location});
                break;
            case syntax::ParameterDeclaration::ParameterMode::Array:
                typeCopy = IntrusiveVariantValue(ArrayType{baseTypePtr, {}});
                break;
            case syntax::ParameterDeclaration::ParameterMode::Label:
                typeCopy = IntrusiveVariantValue(LabelType{});
                break;
            case syntax::ParameterDeclaration::ParameterMode::Switch:
                typeCopy = IntrusiveVariantValue(SwitchType{});
                break;
            case syntax::ParameterDeclaration::ParameterMode::Procedure:
            {
                const Type* procReturnType = nullptr;
                if (node.getType())
                {
                    m_context.ownedTypes.push_back(createTypeFromToken(
                        node.getType(), node.getFixedTotalBits(), node.getFixedFractionBits()));
                    procReturnType = m_context.ownedTypes.back().data();
                }
                typeCopy = IntrusiveVariantValue(ProcedureType{procReturnType, {}});
                break;
            }
            case syntax::ParameterDeclaration::ParameterMode::UntypedValue:
                typeCopy = IntrusiveVariantValue(UntypedType{Type::Value});
                break;
            case syntax::ParameterDeclaration::ParameterMode::UntypedLocation:
                typeCopy = IntrusiveVariantValue(UntypedType{Type::Location});
                break;
        }

        auto declaration = makeIntrusiveVariant<VariableDeclaration>(id, std::move(typeCopy));
        declaration->setAstNode(&node);
        const auto& name = std::get<std::string>(id->getValue());

        if (m_context.symbolTable.lookupInCurrentScope(name))
        {
            m_context.log(errors::semantics::REDEFINITION_OF_SYMBOL_N.args(*id, m_context.source, *id));
            DiagnosticCollector::addDiagnostic(errors::semantics::REDEFINITION_OF_SYMBOL_N);
        }
        else
        {
            if (const Usable* existing = m_context.symbolTable.lookup(name))
            {
                existing->match(
                    [&](const auto& existingDecl)
                    {
                        declaration->setCommunicatorType(existingDecl.getCommunicatorType());
                        if (existingDecl.getAbsoluteAddress().has_value())
                            declaration->setAbsoluteAddress(existingDecl.getAbsoluteAddress().value());
                    });
            }
            declaration->setScopeDepth(m_context.symbolTable.scopeCount());
            m_context.symbolTable.insert(name, declaration.get());
        }
        m_context.ownedUsables.push_back(std::move(declaration));
    }
}

void SymbolResolutionPass::visit(const syntax::Expression& node)
{
    if (auto* idExpr = dynamic_cast<const syntax::IdentifierExpression*>(&node))
        visit(*idExpr);
    else if (auto* binExpr = dynamic_cast<const syntax::BinaryExpression*>(&node))
        visit(*binExpr);
    else if (auto* unExpr = dynamic_cast<const syntax::UnaryExpression*>(&node))
        visit(*unExpr);
    else if (auto* pwExpr = dynamic_cast<const syntax::PartWordExpression*>(&node))
        visit(*pwExpr);
    else if (auto* condExpr = dynamic_cast<const syntax::ConditionalExpression*>(&node))
        visit(*condExpr);
    else if (auto* callExpr = dynamic_cast<const syntax::FunctionCallExpression*>(&node))
        visit(*callExpr);
    else if (auto* arrExpr = dynamic_cast<const syntax::ArrayAccessExpression*>(&node))
        visit(*arrExpr);
    else if (auto* locExpr = dynamic_cast<const syntax::LocationExpression*>(&node))
        visit(*locExpr);
    else if (auto* tyExpr = dynamic_cast<const syntax::TypeChangeExpression*>(&node))
        visit(*tyExpr);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void SymbolResolutionPass::visit(const syntax::IdentifierExpression& node)
{
    const auto& name = std::get<std::string>(node.getIdentifier()->getValue());
    if (const Usable* usable = m_context.symbolTable.lookup(name); !usable)
    {
        m_context.log(errors::semantics::UNDECLARED_IDENTIFIER_N.args(*node.getIdentifier(), m_context.source,
                                                                      *node.getIdentifier()));
        DiagnosticCollector::addDiagnostic(errors::semantics::UNDECLARED_IDENTIFIER_N);
    }
    else
    {
        m_context.resolvedIdentifiers[&node] = usable;
    }
}

void SymbolResolutionPass::visit(const syntax::BinaryExpression& node)
{
    visit(node.getLeft());
    visit(node.getRight());
}

void SymbolResolutionPass::visit(const syntax::UnaryExpression& node)
{
    visit(node.getOperand());
}

void SymbolResolutionPass::visit(const syntax::PartWordExpression& node)
{
    visit(node.getBase());
    visit(node.getStartBit());
    visit(node.getWidth());
}

void SymbolResolutionPass::visit(const syntax::ConditionalExpression& node)
{
    visit(node.getCondition());
    visit(node.getThenBranch());
    visit(node.getElseBranch());
}

void SymbolResolutionPass::visit(const syntax::FunctionCallExpression& node)
{
    const auto& name = std::get<std::string>(node.getFunction()->getValue());
    if (const Usable* usable = m_context.symbolTable.lookup(name); !usable)
    {
        m_context.log(errors::semantics::UNDECLARED_IDENTIFIER_N.args(*node.getFunction(), m_context.source,
                                                                      *node.getFunction()));
        DiagnosticCollector::addDiagnostic(errors::semantics::UNDECLARED_IDENTIFIER_N);
    }
    else
    {
        m_context.resolvedIdentifiers[&node] = usable;
    }

    for (const auto& arg : node.getArguments())
    {
        visit(*arg);
    }
}

void SymbolResolutionPass::visit(const syntax::ArrayAccessExpression& node)
{
    if (node.getArray())
    {
        const auto& name = std::get<std::string>(node.getArray()->getValue());
        if (const Usable* usable = m_context.symbolTable.lookup(name); !usable)
        {
            m_context.log(
                errors::semantics::UNDECLARED_IDENTIFIER_N.args(*node.getArray(), m_context.source, *node.getArray()));
            DiagnosticCollector::addDiagnostic(errors::semantics::UNDECLARED_IDENTIFIER_N);
        }
        else
        {
            m_context.resolvedIdentifiers[&node] = usable;
        }
    }
    for (const auto& idx : node.getIndices())
    {
        visit(*idx);
    }
}

void SymbolResolutionPass::visit(const syntax::LocationExpression& node)
{
    visit(node.getWordReference());
}

void SymbolResolutionPass::visit(const syntax::TypeChangeExpression& node)
{
    if (node.getTotalBits())
        visit(*node.getTotalBits());
    if (node.getFractionBits())
        visit(*node.getFractionBits());
    visit(node.getExpression());
}

// ReSharper disable once CppMemberFunctionMayBeConst
void SymbolResolutionPass::visit(const syntax::CodeStatement& node)
{
    const auto& codeToken = node.getCode();
    const auto& codeStrVariant = codeToken->getValue();
    if (!std::holds_alternative<std::string>(codeStrVariant))
        return;

    const auto rawCode = std::get<std::string>(codeStrVariant);
    ParsedCode parsed;

    struct TokenInfo
    {
        bool isOutput;
        std::string name;
        size_t pos;
    };
    std::vector<TokenInfo> tokens;

    size_t i = 0;
    while (i < rawCode.length())
    {
        if (rawCode[i] == '@' || rawCode[i] == '!')
        {
            const bool isOutput = rawCode[i] == '!';
            const size_t pos = i;
            i++;
            std::string idStr;
            while (i < rawCode.length() && (std::isalnum(rawCode[i]) || rawCode[i] == '_'))
            {
                idStr += rawCode[i];
                i++;
            }
            if (!idStr.empty())
            {
                tokens.push_back({.isOutput = isOutput, .name = idStr, .pos = pos});
            }
        }
        else
        {
            i++;
        }
    }

    std::string llvmCode;
    size_t lastPos = 0;
    size_t numOutputs = 0;
    for (const auto& t : tokens)
        if (t.isOutput)
            numOutputs++;

    size_t outIdx = 0;
    size_t inIdx = numOutputs;

    for (const auto& [isOutput, name, pos] : tokens)
    {
        llvmCode += rawCode.substr(lastPos, pos - lastPos);
        if (const Usable* symbol = m_context.symbolTable.lookup(name); !symbol)
        {
            m_context.log(errors::semantics::UNDECLARED_IDENTIFIER_N.args(*codeToken, m_context.source, *codeToken));
            DiagnosticCollector::addDiagnostic(errors::semantics::UNDECLARED_IDENTIFIER_N);
        }
        else
        {
            if (isOutput)
            {
                parsed.outputs.push_back(symbol);
                llvmCode += "$" + std::to_string(outIdx++);
            }
            else
            {
                parsed.inputs.push_back(symbol);
                llvmCode += "$" + std::to_string(inIdx++);
            }
        }
        lastPos = pos + 1 + name.length(); // 1 for @ or !
    }
    llvmCode += rawCode.substr(lastPos);

    parsed.llvmCode = llvmCode;
    m_context.parsedCodes[&node] = parsed;
}
} // namespace coral66::semantics
