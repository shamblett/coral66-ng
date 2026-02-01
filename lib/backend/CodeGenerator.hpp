/*
* Project: coral66
* Author : Copyright S. Hamblett <steve.hamblett@linux.com>
* Date   : 31/01/2026
*/

#pragma once

#include <llvm/IR/DIBuilder.h>
#include <llvm/IR/IRBuilder.h>
#include "frontend/compiler/Program.hpp"
#include "frontend/compiler/Semantics.hpp"
#include "support/Filesystem.hpp"

#include "CodeGeneration.hpp"

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

class CodeGenerator final
{
    llvm::Module& m_module;
    const Semantics::Program& m_program;
    const SourceInterface& m_sourceInterface;
    const CGLLVM::Options& m_options;

    llvm::IRBuilder<> m_builder{m_module.getContext()};
    llvm::Function* m_currentFunction = nullptr;
    const Semantics::ProcedureDeclaration* m_currentProcedure = nullptr;

    std::unordered_map<const Semantics::Useable*, llvm::Value*> m_globals;
    std::unordered_map<const Semantics::Useable*, llvm::AllocaInst*> m_locals;
    std::unordered_map<const Semantics::LabelStatement*, llvm::BasicBlock*> m_labels;

public:
    explicit CodeGenerator(llvm::Module& module, const Semantics::Program& program,
                           const SourceInterface& sourceInterface, const CGLLVM::Options& options);

    void generate();

    // Type Lowering
    llvm::Type* convertType(const Semantics::Type& type);

    // Helpers
    llvm::Value* emitAddress(const Semantics::ExpressionBase& expr);
    llvm::BasicBlock* getOrCreateLabel(const Semantics::LabelStatement* label);

    // Visitors (Top Level)
    void visit(const Semantics::Declaration& decl);
    
    // Visitors (Statements)
    void visit(const Semantics::Statement& stmt);
    void visit(const Semantics::AssignmentStatement& stmt);
    void visit(const Semantics::IfStatement& stmt);
    void visit(const Semantics::GotoStatement& stmt);
    void visit(const Semantics::ForStatement& stmt);
    void visit(const Semantics::AnswerStatement& stmt);
    void visit(const Semantics::CodeStatement& stmt);
    void visit(const Semantics::ProcedureCallStatement& stmt);
    void visit(const Semantics::LabelStatement& stmt); // Add visitor for LabelStatement

    // Visitors (Expressions)
    llvm::Value* visit(const Semantics::ExpressionBase& expr);
    llvm::Value* visit(const Semantics::Constant& expr);
    llvm::Value* visit(const Semantics::DeclarationRead& expr);
    llvm::Value* visit(const Semantics::BinaryOperator& expr);
    llvm::Value* visit(const Semantics::UnaryOperator& expr);
    llvm::Value* visit(const Semantics::CallExpression& expr);
    llvm::Value* visit(const Semantics::PartWordAccess& expr);
    llvm::Value* visit(const Semantics::ErrorExpression& expr);

private:
    void generateProcedure(const Semantics::ProcedureDeclaration& procDecl);
};

} // namespace coral66::CGLLVM
