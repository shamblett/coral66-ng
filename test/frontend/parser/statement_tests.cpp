/*
 * Project: coral66-ng-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 07/03/2026
 */

#include "lib/frontend/syntax/syntax.hpp"
#include "llvm/ADT/APSInt.h"
#include "test/frontend/common/common.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

using namespace coral66;
using namespace coral66::syntax;

/*
 * Tests cover section 7 of the specification
 */

/*
 * 7.1 Assignment
 */
TEST_CASE("Coral Parse Statements - Assignment", "[Parser][Statement]")
{
    SECTION("Simple")
    {
        const ParsedStatement s("A := 2;");
        auto assignmentStatement = dynamic_cast<AssignmentStatement*>(s.statement.get());
        REQUIRE(assignmentStatement != nullptr);
        CHECK(std::get<llvm::APSInt>(assignmentStatement->getExpression().begin()->getValue()) == 2);
    }

    SECTION("Expression")
    {
        const ParsedStatement s("A := IF 2 > 1 THEN 3 ELSE 4;");
        auto assignmentStatement = dynamic_cast<AssignmentStatement*>(s.statement.get());
        REQUIRE(assignmentStatement != nullptr);
        auto condExpr = dynamic_cast<const ConditionalExpression*>(&assignmentStatement->getExpression());
        CHECK(condExpr != nullptr);

        auto thenBranch = dynamic_cast<const LiteralExpression*>(&condExpr->getThenBranch());
        CHECK(thenBranch != nullptr);
        CHECK(std::get<llvm::APSInt>(thenBranch->getLiteral()->getValue()) == 3);

        auto elseBranch = dynamic_cast<const LiteralExpression*>(&condExpr->getElseBranch());
        CHECK(elseBranch != nullptr);
        CHECK(std::get<llvm::APSInt>(elseBranch->getLiteral()->getValue()) == 4);
    }

    SECTION("Location")
    {
        const ParsedStatement s("[LOCATION(i) + 1] := 3.8;");
        auto assignmentStatement = dynamic_cast<AssignmentStatement*>(s.statement.get());
        REQUIRE(assignmentStatement != nullptr);
    }

    SECTION("Bits")
    {
        const ParsedStatement s("BITS[2,6]x := 3;");
        auto assignmentStatement = dynamic_cast<AssignmentStatement*>(s.statement.get());
        REQUIRE(assignmentStatement != nullptr);
    }

    SECTION("Location And Bits")
    {
        const ParsedStatement s("BITS[1,23] [LOCATION(i) + 1] := 1;");
        auto assignmentStatement = dynamic_cast<AssignmentStatement*>(s.statement.get());
        REQUIRE(assignmentStatement != nullptr);
    }
}

/*
 * 7.2 Goto
 */
TEST_CASE("Coral Parse Statements - Goto", "[Parser][Statement]")
{
    SECTION("Label")
    {
        const ParsedStatement s("GOTO FRED ;");
        auto gotoStatement = dynamic_cast<GotoStatement*>(s.statement.get());
        CHECK(gotoStatement != nullptr);
        CHECK(gotoStatement->getLabel()->getText() == "FRED");
        CHECK(gotoStatement->getIndex() == nullptr);
    }

    SECTION("Switch")
    {
        const ParsedStatement s("GOTO s[0] ;");
        auto gotoStatement = dynamic_cast<GotoStatement*>(s.statement.get());
        CHECK(gotoStatement != nullptr);
        CHECK(gotoStatement->getLabel()->getText() == "s");
        CHECK(gotoStatement->getIndex() != nullptr);
        auto indexConst = dynamic_cast<const LiteralExpression*>(gotoStatement->getIndex());
        CHECK(indexConst != nullptr);
        CHECK(std::get<llvm::APSInt>(indexConst->getLiteral()->getValue()) == 0);
    }
}

/*
 * 7.3 Procedure call
 */
TEST_CASE("Coral Parse Statements - Procedure", "[Parser][Statement]")
{
    SECTION("No Actual Parameters")
    {
        const ParsedStatement s("P;");
        auto procedureCallStatement = dynamic_cast<ProcedureCallStatement*>(s.statement.get());
        CHECK(procedureCallStatement != nullptr);
        CHECK(std::get<std::string>(procedureCallStatement->getExpression().begin()->getValue()) == "P");
    }

    SECTION("Actual Parameters - Simple")
    {
        const ParsedStatement s("P(1,2,3);");
        auto procedureCallStatement = dynamic_cast<ProcedureCallStatement*>(s.statement.get());
        CHECK(procedureCallStatement != nullptr);
        auto expressionPtr = procedureCallStatement->getExpression().begin();
        CHECK(std::get<std::string>(expressionPtr->getValue()) == "P");
        expressionPtr++;
        expressionPtr++;
        CHECK(std::get<llvm::APSInt>(expressionPtr->getValue()) == 1);
        expressionPtr++;
        expressionPtr++;
        CHECK(std::get<llvm::APSInt>(expressionPtr->getValue()) == 2);
        expressionPtr++;
        expressionPtr++;
        CHECK(std::get<llvm::APSInt>(expressionPtr->getValue()) == 3);
    }

    SECTION("Actual Parameters - Complex")
    {
        const ParsedStatement s("P(1 + 2,2 * 6,3);");
        auto procedureCallStatement = dynamic_cast<ProcedureCallStatement*>(s.statement.get());
        CHECK(procedureCallStatement != nullptr);
        auto expressionPtr = procedureCallStatement->getExpression().begin();
        CHECK(std::get<std::string>(expressionPtr->getValue()) == "P");
        expressionPtr++;
        expressionPtr++;
        CHECK(std::get<llvm::APSInt>(expressionPtr->getValue()) == 1);
        expressionPtr++;
        CHECK(expressionPtr->getTokenType() == lexer::TokenType::Plus);
        expressionPtr++;
        CHECK(std::get<llvm::APSInt>(expressionPtr->getValue()) == 2);
        expressionPtr++;
        expressionPtr++;
        CHECK(std::get<llvm::APSInt>(expressionPtr->getValue()) == 2);
        expressionPtr++;
        CHECK(expressionPtr->getTokenType() == lexer::TokenType::Asterisk);
        expressionPtr++;
        CHECK(std::get<llvm::APSInt>(expressionPtr->getValue()) == 6);
        expressionPtr++;
        expressionPtr++;
        CHECK(std::get<llvm::APSInt>(expressionPtr->getValue()) == 3);
    }

    SECTION("Actual Parameters - Word Reference")
    {
        const ParsedStatement s("P([1], W[2]);");
        auto procedureCallStatement = dynamic_cast<ProcedureCallStatement*>(s.statement.get());
        CHECK(procedureCallStatement != nullptr);
        auto expressionPtr = procedureCallStatement->getExpression().begin();
        CHECK(std::get<std::string>(expressionPtr->getValue()) == "P");
        expressionPtr++;
        expressionPtr++;
        CHECK(expressionPtr->getTokenType() == lexer::TokenType::OpenSquareBracket);
        expressionPtr++;
        CHECK(std::get<llvm::APSInt>(expressionPtr->getValue()) == 1);
        expressionPtr++;
        CHECK(expressionPtr->getTokenType() == lexer::TokenType::CloseSquareBracket);
        expressionPtr++;
        expressionPtr++;
        CHECK(std::get<std::string>(expressionPtr->getValue()) == "W");
        expressionPtr++;
        CHECK(expressionPtr->getTokenType() == lexer::TokenType::OpenSquareBracket);
        expressionPtr++;
        CHECK(std::get<llvm::APSInt>(expressionPtr->getValue()) == 2);
        expressionPtr++;
        CHECK(expressionPtr->getTokenType() == lexer::TokenType::CloseSquareBracket);
    }

    SECTION("Actual Parameters - Destination")
    {
        const ParsedStatement s("P(Label1, S[2]);");
        auto procedureCallStatement = dynamic_cast<ProcedureCallStatement*>(s.statement.get());
        CHECK(procedureCallStatement != nullptr);
        auto expressionPtr = procedureCallStatement->getExpression().begin();
        CHECK(std::get<std::string>(expressionPtr->getValue()) == "P");
        expressionPtr++;
        expressionPtr++;
        CHECK(std::get<std::string>(expressionPtr->getValue()) == "Label1");
        expressionPtr++;
        expressionPtr++;
        CHECK(std::get<std::string>(expressionPtr->getValue()) == "S");
        expressionPtr++;
        CHECK(expressionPtr->getTokenType() == lexer::TokenType::OpenSquareBracket);
        expressionPtr++;
        CHECK(std::get<llvm::APSInt>(expressionPtr->getValue()) == 2);
        expressionPtr++;
        CHECK(expressionPtr->getTokenType() == lexer::TokenType::CloseSquareBracket);
    }

    SECTION("Actual Parameters - Name")
    {
        const ParsedStatement s("P(A);");
        auto procedureCallStatement = dynamic_cast<ProcedureCallStatement*>(s.statement.get());
        CHECK(procedureCallStatement != nullptr);
        auto expressionPtr = procedureCallStatement->getExpression().begin();
        CHECK(std::get<std::string>(expressionPtr->getValue()) == "P");
        expressionPtr++;
        expressionPtr++;
        CHECK(std::get<std::string>(expressionPtr->getValue()) == "A");
    }
    SECTION("No Semicolon Required")
    {
        const ParsedStatement s("P");
        auto procedureCallStatement = dynamic_cast<ProcedureCallStatement*>(s.statement.get());
        CHECK(procedureCallStatement != nullptr);
        CHECK(std::get<std::string>(procedureCallStatement->getExpression().begin()->getValue()) == "P");
    }

    SECTION("Procedure Call At End Of Block Without Semicolon")
    {
        const ParsedStatement s("BEGIN P END;");
        auto compStmt = dynamic_cast<CompoundStatement*>(s.statement.get());
        CHECK(compStmt != nullptr);
        CHECK(compStmt->getStatements().size() == 1);
        auto procCall = dynamic_cast<ProcedureCallStatement*>(compStmt->getStatements()[0].get());
        CHECK(procCall != nullptr);
    }
}

/*
 * 7.4 Answer
 */
TEST_CASE("Coral Parse Statements - Answer", "[Parser][Statement]")
{
    SECTION("Normal Answer in Typed Procedure")
    {
        const ParsedStatement s("ANSWER B;", true, false);
        auto answerStatement = dynamic_cast<AnswerStatement*>(s.statement.get());
        CHECK(answerStatement != nullptr);
        CHECK(std::get<std::string>(answerStatement->getExpression().begin()->getValue()) == "B");
    }
}

/*
 * 7.5 Code
 */
TEST_CASE("Coral Parse Statements - Code", "[Parser][Statement]")
{
    SECTION("No substitutions")
    {
        const ParsedStatement s("CODE BEGIN \"mov eax, add eax, mov eax\" END;");
        auto codeStatement = dynamic_cast<CodeStatement*>(s.statement.get());
        CHECK(codeStatement != nullptr);
        CHECK(std::get<std::string>(codeStatement->getCode()->getValue()) == "mov eax, add eax, mov eax");
    }

    SECTION("Substitutions")
    {
        const ParsedStatement s("CODE BEGIN \"mov eax, add @v1, mov eax !result\" END;");
        auto codeStatement = dynamic_cast<CodeStatement*>(s.statement.get());
        CHECK(codeStatement != nullptr);
        CHECK(std::get<std::string>(codeStatement->getCode()->getValue()) == "mov eax, add @v1, mov eax !result");
    }
}

/*
 * 7.6 Compound
 */
TEST_CASE("Coral Parse Statements - Compound", "[Parser][Statement]")
{
    SECTION("Empty compound")
    {
        const ParsedStatement s("BEGIN END;");
        auto compoundStatement = dynamic_cast<CompoundStatement*>(s.statement.get());
        CHECK(compoundStatement != nullptr);
        CHECK(compoundStatement->getStatements().empty());
    }

    SECTION("Semicolon separator compound")
    {
        const ParsedStatement s("BEGIN ; END;");
        auto compoundStatement = dynamic_cast<CompoundStatement*>(s.statement.get());
        CHECK(compoundStatement != nullptr);
        CHECK(compoundStatement->getStatements().size() == 1);
        auto dummy = dynamic_cast<DummyStatement*>(compoundStatement->getStatements()[0].get());
        CHECK(dummy != nullptr);
    }

    SECTION("Declarations after statements error")
    {
        const ParsedStatement s("BEGIN A := 1; INTEGER B; B := 2 END;");
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
    }

    SECTION("Single statement compound")
    {
        const ParsedStatement s("BEGIN A := 1 END;");
        auto compoundStatement = dynamic_cast<CompoundStatement*>(s.statement.get());
        CHECK(compoundStatement != nullptr);
        CHECK(compoundStatement->getStatements().size() == 1);
        auto* assign = dynamic_cast<AssignmentStatement*>(compoundStatement->getStatements()[0].get());
        CHECK(assign != nullptr);
    }

    SECTION("Multi statement compound")
    {
        const ParsedStatement s("BEGIN A := 1; B := 2 END;");
        auto compoundStatement = dynamic_cast<CompoundStatement*>(s.statement.get());
        CHECK(compoundStatement != nullptr);
        CHECK(compoundStatement->getStatements().size() == 2);
    }

    SECTION("Block as statement")
    {
        const ParsedStatement s("BEGIN INTEGER A; A := 1 END;");
        auto blockStatement = dynamic_cast<BlockStatement*>(s.statement.get());
        CHECK(blockStatement != nullptr);
        const auto& block = blockStatement->getBlock();
        CHECK(block.getDeclarations().size() == 1);
        CHECK(block.getStatements().size() == 1);
    }
}

/*
 * 7.7 Blocks
 */

TEST_CASE("Coral Parse Statements - Blocks and Declarations", "[Parser][Statement][Block]")
{
    SECTION("Single Label")
    {
        const ParsedStatement s("L1: ANSWER B;", true, false);
        auto answerStatement = dynamic_cast<AnswerStatement*>(s.statement.get());
        CHECK(answerStatement != nullptr);
        CHECK(answerStatement->getLabels().size() == 1);
        CHECK(std::get<std::string>(answerStatement->getLabels()[0]->getValue()) == "L1");
    }
    SECTION("Multiple Labels")
    {
        const ParsedStatement s("L1: L2: L3: ANSWER B;", true, false);
        auto answerStatement = dynamic_cast<AnswerStatement*>(s.statement.get());
        CHECK(answerStatement != nullptr);
        CHECK(answerStatement->getLabels().size() == 3);
        CHECK(std::get<std::string>(answerStatement->getLabels()[0]->getValue()) == "L1");
        CHECK(std::get<std::string>(answerStatement->getLabels()[1]->getValue()) == "L2");
        CHECK(std::get<std::string>(answerStatement->getLabels()[2]->getValue()) == "L3");
    }
}

/*
 * 7.8 Dummy
 */
TEST_CASE("Coral Parse Statements - Dummy", "[Parser][Statement]")
{
    SECTION("After Label")
    {
        const ParsedStatement s("; label: END");
        auto dummyStatement = dynamic_cast<DummyStatement*>(s.statement.get());
        CHECK(dummyStatement != nullptr);
    }

    SECTION("End")
    {
        const ParsedStatement s("END");
        auto dummyStatement = dynamic_cast<DummyStatement*>(s.statement.get());
        CHECK(dummyStatement != nullptr);
    }

    SECTION("Else")
    {
        const ParsedStatement s("ELSE");
        auto dummyStatement = dynamic_cast<DummyStatement*>(s.statement.get());
        CHECK(dummyStatement != nullptr);
    }
}

/*
 * 7.9 Conditional
 */
TEST_CASE("Coral Parse Statements - Conditional", "[Parser][Statement]")
{
    SECTION("If Then - Simple Statement")
    {
        const ParsedStatement s("IF A = 1 THEN B := 2;");
        auto ifStatement = dynamic_cast<IfStatement*>(s.statement.get());
        CHECK(ifStatement != nullptr);
    }

    SECTION("If Then - Labeled Simple Statement Allowed")
    {
        const ParsedStatement s("IF A = 1 THEN L1: B := 2;");
        auto ifStatement = dynamic_cast<IfStatement*>(s.statement.get());
        CHECK(ifStatement != nullptr);
    }

    SECTION("If Then Else - Else Can Have If")
    {
        const ParsedStatement s("IF A = 1 THEN B := 2 ELSE IF B = 3 THEN C := 4;");
        auto ifStatement = dynamic_cast<IfStatement*>(s.statement.get());
        CHECK(ifStatement != nullptr);
    }

    SECTION("If Then Else - Else Can Have For")
    {
        const ParsedStatement s("IF A = 1 THEN B := 2 ELSE FOR I := 1 STEP 1 UNTIL 10 DO C := 4;");
        auto ifStatement = dynamic_cast<IfStatement*>(s.statement.get());
        CHECK(ifStatement != nullptr);
    }

    SECTION("If Then - Nested If Allowed in Block")
    {
        const ParsedStatement s("IF A = 1 THEN BEGIN IF B = 2 THEN C := 3 END;");
        auto ifStatement = dynamic_cast<IfStatement*>(s.statement.get());
        CHECK(ifStatement != nullptr);
    }
    SECTION("If Then - Statement")
    {
        const ParsedStatement s("IF A < 2 THEN C := 3");
        auto ifStatement = dynamic_cast<IfStatement*>(s.statement.get());
        CHECK(ifStatement != nullptr);
    }

    SECTION("If Then Else - Statement")
    {
        const ParsedStatement s("IF A < 2 THEN C := 3 ELSE C := 4");
        auto ifStatement = dynamic_cast<IfStatement*>(s.statement.get());
        CHECK(ifStatement != nullptr);
    }
}

/*
 * 7.10 For
 */
TEST_CASE("Coral Parse Statements - For", "[Parser][Statement]")
{
    SECTION("FOR Single STEP DO")
    {
        const ParsedStatement s("FOR A := 1 STEP 1 UNTIL 4 DO B := B + 2");
        auto forStatement = dynamic_cast<ForStatement*>(s.statement.get());
        CHECK(forStatement != nullptr);
        CHECK(std::get<std::string>(forStatement->getVariable().begin()->getValue()) == "A");
        CHECK(forStatement->getElements().size() == 1);
        auto& el = forStatement->getElements()[0];
        CHECK(std::get<llvm::APSInt>(el.expression->begin()->getValue()) == 1);
        CHECK(std::get<llvm::APSInt>(el.step->begin()->getValue()) == 1);
        CHECK(std::get<llvm::APSInt>(el.until->begin()->getValue()) == 4);
        CHECK(std::get<std::string>(forStatement->getBody().begin()->getValue()) == "B");
        CHECK(std::get<std::string>((forStatement->getBody().begin() + 2)->getValue()) == "B");
        CHECK(std::get<llvm::APSInt>((forStatement->getBody().begin() + 4)->getValue()) == 2);
    }

    SECTION("FOR Double STEP DO")
    {
        const ParsedStatement s("FOR A := 1 STEP 1 UNTIL 4, "
                                "6 STEP 2 UNTIL 10 "
                                "DO B := B + 2");
        auto forStatement = dynamic_cast<ForStatement*>(s.statement.get());
        CHECK(forStatement != nullptr);
        CHECK(std::get<std::string>(forStatement->getVariable().begin()->getValue()) == "A");
        CHECK(forStatement->getElements().size() == 2);
        auto& el1 = forStatement->getElements()[0];
        CHECK(std::get<llvm::APSInt>(el1.expression->begin()->getValue()) == 1);
        CHECK(std::get<llvm::APSInt>(el1.step->begin()->getValue()) == 1);
        CHECK(std::get<llvm::APSInt>(el1.until->begin()->getValue()) == 4);

        auto& el2 = forStatement->getElements()[1];
        CHECK(std::get<llvm::APSInt>(el2.expression->begin()->getValue()) == 6);
        CHECK(std::get<llvm::APSInt>(el2.step->begin()->getValue()) == 2);
        CHECK(std::get<llvm::APSInt>(el2.until->begin()->getValue()) == 10);

        CHECK(std::get<std::string>(forStatement->getBody().begin()->getValue()) == "B");
    }

    SECTION("FOR Single Expression DO")
    {
        const ParsedStatement s("FOR A := 1 DO B := B + 2");
        auto forStatement = dynamic_cast<ForStatement*>(s.statement.get());
        CHECK(forStatement != nullptr);
        CHECK(std::get<std::string>(forStatement->getVariable().begin()->getValue()) == "A");
        CHECK(forStatement->getElements().size() == 1);
        const auto& [expression, step, until, whileCondition] = forStatement->getElements()[0];
        CHECK(std::get<llvm::APSInt>(expression->begin()->getValue()) == 1);
        CHECK(step == nullptr);
        CHECK(until == nullptr);
        CHECK(whileCondition == nullptr);
    }

    SECTION("FOR Multiple Expressions DO")
    {
        const ParsedStatement s("FOR A := 1, 2, 4 DO B := B + 2");
        auto forStatement = dynamic_cast<ForStatement*>(s.statement.get());
        CHECK(forStatement != nullptr);
        CHECK(std::get<std::string>(forStatement->getVariable().begin()->getValue()) == "A");
        CHECK(forStatement->getElements().size() == 3);
        CHECK(std::get<llvm::APSInt>(forStatement->getElements()[0].expression->begin()->getValue()) == 1);
        CHECK(std::get<llvm::APSInt>(forStatement->getElements()[1].expression->begin()->getValue()) == 2);
        CHECK(std::get<llvm::APSInt>(forStatement->getElements()[2].expression->begin()->getValue()) == 4);
    }

    SECTION("FOR WHILE DO")
    {
        const ParsedStatement s("FOR A := 1 WHILE 1 = 1 DO B := B + 2");
        auto forStatement = dynamic_cast<ForStatement*>(s.statement.get());
        CHECK(forStatement != nullptr);
        CHECK(std::get<std::string>(forStatement->getVariable().begin()->getValue()) == "A");
        CHECK(forStatement->getElements().size() == 1);
        auto& el = forStatement->getElements()[0];
        CHECK(std::get<llvm::APSInt>(el.expression->begin()->getValue()) == 1);
        CHECK(el.whileCondition != nullptr);
        CHECK(std::get<llvm::APSInt>(el.whileCondition->begin()->getValue()) == 1);
    }

    SECTION("FOR Mixed List DO")
    {
        const ParsedStatement s("FOR A := 1, 2 STEP 1 UNTIL 4, 5 WHILE 0 = 0 DO B := B + 2");
        auto forStatement = dynamic_cast<ForStatement*>(s.statement.get());
        CHECK(forStatement != nullptr);
        CHECK(forStatement->getElements().size() == 3);

        auto& el1 = forStatement->getElements()[0];
        CHECK(std::get<llvm::APSInt>(el1.expression->begin()->getValue()) == 1);
        CHECK(el1.step == nullptr);

        auto& el2 = forStatement->getElements()[1];
        CHECK(std::get<llvm::APSInt>(el2.expression->begin()->getValue()) == 2);
        CHECK(std::get<llvm::APSInt>(el2.step->begin()->getValue()) == 1);
        CHECK(std::get<llvm::APSInt>(el2.until->begin()->getValue()) == 4);

        auto& el3 = forStatement->getElements()[2];
        CHECK(std::get<llvm::APSInt>(el3.expression->begin()->getValue()) == 5);
        CHECK(el3.whileCondition != nullptr);
        CHECK(std::get<llvm::APSInt>(el3.whileCondition->begin()->getValue()) == 0);
    }

    SECTION("FOR Array Access DO")
    {
        const ParsedStatement s("FOR A[1] := 1 DO B := B + 2");
        auto forStatement = dynamic_cast<ForStatement*>(s.statement.get());
        CHECK(forStatement != nullptr);
        CHECK(std::get<std::string>(forStatement->getVariable().begin()->getValue()) == "A");
    }

    SECTION("FOR WHILE Condition DO")
    {
        const ParsedStatement s("FOR A := 1 WHILE B = 2 DO B := B + 2");
        auto forStatement = dynamic_cast<ForStatement*>(s.statement.get());
        CHECK(forStatement != nullptr);
        CHECK(forStatement->getElements().size() == 1);
        auto& el = forStatement->getElements()[0];
        CHECK(el.whileCondition != nullptr);
    }
}
