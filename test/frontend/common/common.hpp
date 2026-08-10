/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 07/03/2026
 */

#pragma once

#include <llvm/Support/raw_ostream.h>

#include <utility>

#include "lib/frontend/common/source_object.hpp"
#include "lib/frontend/lexer/lexer.hpp"
#include "lib/frontend/parser/parser.hpp"
#include "lib/frontend/parser/parser_support.hpp"

using namespace coral66;
using namespace coral66::syntax;

struct ParsedExpression
{
    std::string source;
    std::string outStr;
    llvm::raw_string_ostream ss;
    PPSourceObject ppTokens;
    CSourceObject cTokens;
    std::unique_ptr<parser::Context> context;
    std::unique_ptr<Expression> expression;

    explicit ParsedExpression(std::string src) : source(std::move(src)), ss(outStr)
    {
        ppTokens = lexer::tokenize(source, &ss);
        cTokens = lexer::toCTokens(ppTokens, &ss);
        context = std::make_unique<parser::Context>(cTokens, &ss);

        const auto& tokens = cTokens.data();
        const lexer::CToken* begin = tokens.data();
        const lexer::CToken* end = tokens.data() + tokens.size();

        while (begin != end && (end - 1)->getTokenType() == lexer::TokenType::Newline)
        {
            end--;
        }

        parser::DiagnosticCollector::clearDiagnostics();
        expression = parser::parseExpression(begin, end, *context);
    }
};

struct ParsedStatement
{
    std::string source;
    std::string outStr;
    llvm::raw_string_ostream ss;
    PPSourceObject ppTokens;
    CSourceObject cTokens;
    std::unique_ptr<parser::Context> context;
    std::unique_ptr<Statement> statement;

    explicit ParsedStatement(std::string src, const bool isTypedProc = false, const bool isUntypedProc = false)
        : source(std::move(src)), ss(outStr)
    {
        ppTokens = lexer::tokenize(source, &ss);
        cTokens = lexer::toCTokens(ppTokens, &ss);
        context = std::make_unique<parser::Context>(cTokens, &ss);

        if (isTypedProc)
            context->enterProcedure(true);
        if (isUntypedProc)
            context->enterProcedure(false);

        const auto& tokens = cTokens.data();
        const lexer::CToken* begin = tokens.data();
        const lexer::CToken* end = tokens.data() + tokens.size();

        while (begin != end && (end - 1)->getTokenType() == lexer::TokenType::Newline)
        {
            end--;
        }

        parser::DiagnosticCollector::clearDiagnostics();
        statement = parser::parseStatement(begin, end, *context);
    }
};

struct ParsedProcedure
{
    std::string source;
    std::string outStr;
    llvm::raw_string_ostream ss;
    PPSourceObject ppTokens;
    CSourceObject cTokens;
    std::unique_ptr<parser::Context> context;
    std::unique_ptr<ProcedureDeclaration> procedure;

    explicit ParsedProcedure(std::string src) : source(std::move(src)), ss(outStr)
    {
        ppTokens = lexer::tokenize(source, &ss);
        cTokens = lexer::toCTokens(ppTokens, &ss);
        context = std::make_unique<parser::Context>(cTokens, &ss);

        const auto& tokens = cTokens.data();
        const lexer::CToken* begin = tokens.data();
        const lexer::CToken* end = tokens.data() + tokens.size();

        while (begin != end && (end - 1)->getTokenType() == lexer::TokenType::Newline)
        {
            end--;
        }

        parser::DiagnosticCollector::clearDiagnostics();
        procedure = parser::parseProcedure(begin, end, *context);
    }
};

struct ParsedCommunicator
{
    std::string source;
    std::string outStr;
    llvm::raw_string_ostream ss;
    PPSourceObject ppTokens;
    CSourceObject cTokens;
    std::unique_ptr<parser::Context> context;
    std::unique_ptr<Communicator> communicator;

    explicit ParsedCommunicator(std::string src) : source(std::move(src)), ss(outStr)
    {
        ppTokens = lexer::tokenize(source, &ss);
        cTokens = lexer::toCTokens(ppTokens, &ss);
        context = std::make_unique<parser::Context>(cTokens, &ss);

        const auto& tokens = cTokens.data();
        const lexer::CToken* begin = tokens.data();
        const lexer::CToken* end = tokens.data() + tokens.size();

        while (begin != end && (end - 1)->getTokenType() == lexer::TokenType::Newline)
        {
            end--;
        }

        parser::DiagnosticCollector::clearDiagnostics();
        communicator = parser::parseCommunicator(begin, end, *context);
    }
};

struct ParsedDeclaration
{
    std::string source;
    std::string outStr;
    llvm::raw_string_ostream ss;
    PPSourceObject ppTokens;
    CSourceObject cTokens;
    std::unique_ptr<parser::Context> context;
    std::vector<std::unique_ptr<Declaration>> declaration;

    explicit ParsedDeclaration(std::string src) : source(std::move(src)), ss(outStr)
    {
        ppTokens = lexer::tokenize(source, &ss);
        cTokens = lexer::toCTokens(ppTokens, &ss);
        context = std::make_unique<parser::Context>(cTokens, &ss);

        const auto& tokens = cTokens.data();
        const lexer::CToken* begin = tokens.data();
        const lexer::CToken* end = tokens.data() + tokens.size();

        while (begin != end && (end - 1)->getTokenType() == lexer::TokenType::Newline)
        {
            end--;
        }

        parser::DiagnosticCollector::clearDiagnostics();
        declaration = parser::parseDeclaration(begin, end, *context);
    }
};

struct ParsedProgram
{
    std::string source;
    std::string outStr;
    llvm::raw_string_ostream ss;
    PPSourceObject ppTokens;
    CSourceObject cTokens;
    std::unique_ptr<parser::Context> context;
    Program program;

    explicit ParsedProgram(std::string src)
        : source(std::move(src)), ss(outStr), program(nullptr, nullptr, nullptr, std::vector<std::unique_ptr<Communicator>>(), nullptr, false)
    {
        ppTokens = lexer::tokenize(source, &ss);
        cTokens = lexer::toCTokens(ppTokens, &ss);
        context = std::make_unique<parser::Context>(cTokens, &ss);
        parser::DiagnosticCollector::clearDiagnostics();
        program = parser::parseProgram(cTokens, &ss);
    }
};
