/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

#include <llvm/IR/IRBuilder.h>

#include "code_generation.hpp"
#include "lib/frontend/semantics/semantics.hpp"
#include "lib/frontend/syntax/syntax.hpp"

/** Internal LLVM Code Generation Implementation
 *
 * This file defines the `CodeGenerator` class, which implements the detailed
 * AST visitor pattern for the Coral 66 compiler. It is responsible for recursively
 * walking the semantically validated Abstract Syntax Tree (AST) and emitting the
 * corresponding LLVM Intermediate Representation (IR) instructions, managing scopes,
 * types, and variables.
 */

namespace coral66::cgllvm
{

struct Value
{
    llvm::Value* value;
    llvm::MaybeAlign alignment;

    explicit Value(llvm::Value* v, const llvm::MaybeAlign a = {}) : value(v), alignment(a) {}
    explicit Value(std::nullptr_t) : value(nullptr) {}
};

class CodeGenerator final
{
    llvm::Module& m_module;
    const syntax::Program& m_tree;
    const semantics::Program& m_program;
    const source_interface& m_sourceInterface;
    const Options& m_options;

    llvm::IRBuilder<> m_builder{m_module.getContext()};
    llvm::Function* m_currentFunction = nullptr;
    const syntax::ProcedureDeclaration* m_currentProcedure = nullptr;

    std::unordered_map<const semantics::Usable*, llvm::Value*> m_globals;
    std::unordered_map<const semantics::Usable*, llvm::Value*> m_locals;
    std::unordered_map<const syntax::Statement*, llvm::BasicBlock*> m_labels;
    std::unordered_map<std::string, llvm::BasicBlock*> m_namedLabels;
    std::vector<const syntax::OverlayDeclaration*> m_globalOverlays;

public:
    explicit CodeGenerator(llvm::Module& module, const syntax::Program& tree, const semantics::Program& program,
                           const source_interface& sourceInterface, const Options& options);

    void generate();

    llvm::Type* convertType(const semantics::Type& type);
    llvm::Value* emitAddress(const syntax::Expression& expression);
    llvm::BasicBlock* getOrCreateLabel(const syntax::Statement* statement);

    void visit(const semantics::Declaration& declaration);

    void visit(const syntax::Program& program);
    void visit(const syntax::Block& block);
    void visit(const syntax::Declaration& declaration);
    void visit(const syntax::VariableDeclaration& declaration);
    void visit(const syntax::ArrayDeclaration& declaration);
    void visit(const syntax::TableDeclaration& declaration);
    void visit(const syntax::OverlayDeclaration& declaration);

    void visit(const syntax::Statement& statement);
    void visit(const syntax::AssignmentStatement& statement);
    void visit(const syntax::IfStatement& statement);
    void visit(const syntax::GotoStatement& statement);
    void visit(const syntax::ForStatement& statement);
    void visit(const syntax::AnswerStatement& statement);
    void visit(const syntax::CodeStatement& statement);
    void visit(const syntax::ProcedureCallStatement& statement);
    void visit(const syntax::BlockStatement& statement);
    void visit(const syntax::CompoundStatement& statement);

    void preScanLabels(const syntax::Statement& statement);

    llvm::Value* visit(const syntax::Expression& expression);
    llvm::Value* visit(const syntax::LiteralExpression& expression);
    llvm::Value* visit(const syntax::IdentifierExpression& expression);
    llvm::Value* visit(const syntax::BinaryExpression& expression);
    llvm::Value* visit(const syntax::UnaryExpression& expression);
    llvm::Value* visit(const syntax::FunctionCallExpression& expression);
    llvm::Value* visit(const syntax::PartWordExpression& expression);
    llvm::Value* visit(const syntax::ArrayAccessExpression& expression);
    llvm::Value* visit(const syntax::TypeChangeExpression& expression);

private:
    static llvm::Constant* parsePresetList(const std::vector<lexer::CTokenIterator>& tokens, llvm::Type* targetType,
                                           std::size_t numElements);

    void generateProcedure(const semantics::ProcedureDeclaration& procedureDeclaration,
                           const syntax::ProcedureDeclaration& astNode);

    llvm::Value* processFixedBinaryExpression(const syntax::BinaryExpression& expression);
};

} // namespace coral66::cgllvm
