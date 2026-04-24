/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

#include <llvm/IR/IRBuilder.h>

#include "code_generation.hpp"
#include "lib/frontend/semantics/semantics.hpp"

namespace coral66::CGLLVM
{

struct Value
{
    llvm::Value* value;
    // Simple alignment handling for now
    llvm::MaybeAlign alignment;

    Value(llvm::Value* v, llvm::MaybeAlign a = {}) : value(v), alignment(a) {}
    Value(std::nullptr_t) : value(nullptr) {}
};

class code_generator final
{
    llvm::Module& m_module;
    const semantics::Program& m_program;
    const source_interface& m_sourceInterface;
    const CGLLVM::Options& m_options;

    llvm::IRBuilder<> m_builder{m_module.getContext()};
    llvm::Function* m_currentFunction = nullptr;
    const semantics::ProcedureDeclaration* m_currentProcedure = nullptr;

    std::unordered_map<const semantics::Usable*, llvm::Value*> m_globals;
    std::unordered_map<const semantics::Usable*, llvm::AllocaInst*> m_locals;
    std::unordered_map<const semantics::LabelStatement*, llvm::BasicBlock*> m_labels;

public:
    explicit code_generator(llvm::Module& module, const semantics::Program& program,
                            const source_interface& sourceInterface, const CGLLVM::Options& options);

    void generate();

    // Type Lowering
    llvm::Type* convertType(const semantics::Type& type);

    // Helpers
    llvm::Value* emitAddress(const semantics::ExpressionBase& expr);
    llvm::BasicBlock* getOrCreateLabel(const semantics::LabelStatement* label);

    // Visitors (Top Level)
    void visit(const semantics::Declaration& decl);

    // Visitors (Statements)
    void visit(const semantics::Statement& stmt);
    void visit(const semantics::AssignmentStatement& stmt);
    void visit(const semantics::IfStatement& stmt);
    void visit(const semantics::GotoStatement& stmt);
    void visit(const semantics::ForStatement& stmt);
    void visit(const semantics::AnswerStatement& stmt);
    void visit(const semantics::CodeStatement& stmt);
    void visit(const semantics::ProcedureCallStatement& stmt);
    void visit(const semantics::LabelStatement& stmt); // Add visitor for LabelStatement
    void visit(const semantics::DummyStatement& stmt);
    void visit(const semantics::BlockStatement& stmt);
    void visit(const semantics::CompoundStatement& stmt);

    // Visitors (Expressions)
    llvm::Value* visit(const semantics::ExpressionBase& expr);
    llvm::Value* visit(const semantics::Constant& expr);
    llvm::Value* visit(const semantics::DeclarationRead& expr);
    llvm::Value* visit(const semantics::BinaryOperator& expr);
    llvm::Value* visit(const semantics::UnaryOperator& expr);
    llvm::Value* visit(const semantics::CallExpression& expr);
    llvm::Value* visit(const semantics::PartWordAccess& expr);
    static llvm::Value* visit(const semantics::ErrorExpression& expr);

private:
    void generateProcedure(const semantics::ProcedureDeclaration& procDecl);
};

} // namespace coral66::CGLLVM
