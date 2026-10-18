/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 07/03/2026
 */

#include "lib/frontend/syntax/syntax.hpp"
#include "test/frontend/common/common.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

using namespace coral66;
using namespace coral66::syntax;

/*
 * Tests cover section 6 of the specification
 */

/*
 * 6.1.1.1
 */
TEST_CASE("Coral Parse Expression - Primaries", "[Parser][Expression]")
{
    SECTION("Literals")
    {
        const ParsedExpression p("42");
        CHECK(dynamic_cast<LiteralExpression*>(p.expression.get()) != nullptr);
    }
    SECTION("Identifiers")
    {
        const ParsedExpression p("myVar");
        CHECK(dynamic_cast<IdentifierExpression*>(p.expression.get()) != nullptr);
    }
    SECTION("Parenthesized Expression")
    {
        const ParsedExpression p("(42)");
        CHECK(dynamic_cast<LiteralExpression*>(p.expression.get()) != nullptr);
    }
}

/*
 * 6.1.1.2
 */
TEST_CASE("Coral Parse Expression - Typed Primaries", "[Parser][Expression]")
{
    SECTION("Type Change")
    {
        ParsedExpression p("INTEGER ( 3.14 )");
        auto tc = dynamic_cast<TypeChangeExpression*>(p.expression.get());
        CHECK(tc != nullptr);
        CHECK(tc->getType()->getTokenType() == lexer::TokenType::IntegerKeyword);
    }
    SECTION("Type Change FIXED with scale")
    {
        ParsedExpression p("FIXED (10, 2) (3.14)");
        auto tc = dynamic_cast<TypeChangeExpression*>(p.expression.get());
        CHECK(tc != nullptr);
        CHECK(tc->getType()->getTokenType() == lexer::TokenType::FixedKeyword);
        CHECK(tc->getTotalBits() != nullptr);
        CHECK(tc->getFractionBits() != nullptr);
    }
    SECTION("Location")
    {
        ParsedExpression p("LOCATION ( x )");
        auto loc = dynamic_cast<LocationExpression*>(p.expression.get());
        CHECK(loc != nullptr);
        CHECK(dynamic_cast<const IdentifierExpression*>(&loc->getWordReference()) != nullptr);
    }
    SECTION("Part Word")
    {
        ParsedExpression p("BITS [ 8, 4 ] x");
        auto pw = dynamic_cast<PartWordExpression*>(p.expression.get());
        CHECK(pw != nullptr);
        CHECK(dynamic_cast<const IdentifierExpression*>(&pw->getBase()) != nullptr);
    }
    SECTION("Octal Literal")
    {
        ParsedExpression p("OCTAL ( 1234 )");
        auto literal = dynamic_cast<LiteralExpression*>(p.expression.get());
        REQUIRE(literal != nullptr);
        CHECK(literal->getLiteral()->getTokenType() == lexer::TokenType::Literal);
        CHECK(literal->getLiteral()->getType() == lexer::CToken::Type::Integer);
        CHECK(std::get<llvm::APSInt>(literal->getLiteral()->getValue()) == 01234);
    }
    SECTION("Octal Point Literal")
    {
        ParsedExpression p("OCTAL ( 12.34 )");
        auto literal = dynamic_cast<LiteralExpression*>(p.expression.get());
        REQUIRE(literal != nullptr);
        CHECK(literal->getLiteral()->getTokenType() == lexer::TokenType::Literal);
        CHECK(literal->getLiteral()->getType() == lexer::CToken::Type::Floating);
        CHECK(std::get<llvm::APFloat>(literal->getLiteral()->getValue()) == llvm::APFloat(10.4375));
    }
    SECTION("Signed Octal Point Literal")
    {
        ParsedExpression p("- OCTAL ( 1.4 )");
        auto unary = dynamic_cast<UnaryExpression*>(p.expression.get());
        REQUIRE(unary != nullptr);
        auto literal = dynamic_cast<const LiteralExpression*>(&unary->getOperand());
        REQUIRE(literal != nullptr);
        CHECK(literal->getLiteral()->getType() == lexer::CToken::Type::Floating);
        CHECK(std::get<llvm::APFloat>(literal->getLiteral()->getValue()) == llvm::APFloat(1.5));
    }
    SECTION("Octal Point Rejected as Typed Primary")
    {
        ParsedExpression p("a MASK OCTAL ( 12.34 )");
        CHECK(p.expression == nullptr);
        CHECK_FALSE(parser::DiagnosticCollector::getDiagnostics().empty());
    }
}

/*
 * 6.1.2
 */
TEST_CASE("Coral Parse Expression - Bitwise", "[Parser][Expression]")
{
    SECTION("MASK")
    {
        ParsedExpression p("a MASK b");
        auto binOp = dynamic_cast<BinaryExpression*>(p.expression.get());
        REQUIRE(binOp != nullptr);
        CHECK(binOp->getOp()->getTokenType() == lexer::TokenType::MaskKeyword);
    }
    SECTION("UNION")
    {
        ParsedExpression p("a UNION b");
        auto binOp = dynamic_cast<BinaryExpression*>(p.expression.get());
        REQUIRE(binOp != nullptr);
        CHECK(binOp->getOp()->getTokenType() == lexer::TokenType::UnionKeyword);
    }
    SECTION("DIFFER")
    {
        ParsedExpression p("a DIFFER b");
        auto binOp = dynamic_cast<BinaryExpression*>(p.expression.get());
        REQUIRE(binOp != nullptr);
        CHECK(binOp->getOp()->getTokenType() == lexer::TokenType::DifferKeyword);
    }
    SECTION("Bitwise Precedence")
    {
        ParsedExpression p("a DIFFER b UNION c MASK d");
        auto diffOp = dynamic_cast<BinaryExpression*>(p.expression.get());
        REQUIRE(diffOp != nullptr);
        CHECK(diffOp->getOp()->getTokenType() == lexer::TokenType::DifferKeyword);
        CHECK(dynamic_cast<const IdentifierExpression*>(&diffOp->getLeft()) != nullptr);

        auto unionOp = dynamic_cast<const BinaryExpression*>(&diffOp->getRight());
        REQUIRE(unionOp != nullptr);
        CHECK(unionOp->getOp()->getTokenType() == lexer::TokenType::UnionKeyword);
        CHECK(dynamic_cast<const IdentifierExpression*>(&unionOp->getLeft()) != nullptr);

        auto maskOp = dynamic_cast<const BinaryExpression*>(&unionOp->getRight());
        REQUIRE(maskOp != nullptr);
        CHECK(maskOp->getOp()->getTokenType() == lexer::TokenType::MaskKeyword);
        CHECK(dynamic_cast<const IdentifierExpression*>(&maskOp->getLeft()) != nullptr);
        CHECK(dynamic_cast<const IdentifierExpression*>(&maskOp->getRight()) != nullptr);
    }
    SECTION("Bitwise Left Associativity")
    {
        ParsedExpression pDiff("a DIFFER b DIFFER c");
        auto diffOuter = dynamic_cast<BinaryExpression*>(pDiff.expression.get());
        REQUIRE(diffOuter != nullptr);
        CHECK(diffOuter->getOp()->getTokenType() == lexer::TokenType::DifferKeyword);
        auto diffInner = dynamic_cast<const BinaryExpression*>(&diffOuter->getLeft());
        REQUIRE(diffInner != nullptr);
        CHECK(diffInner->getOp()->getTokenType() == lexer::TokenType::DifferKeyword);

        ParsedExpression pUnion("a UNION b UNION c");
        auto unionOuter = dynamic_cast<BinaryExpression*>(pUnion.expression.get());
        REQUIRE(unionOuter != nullptr);
        CHECK(unionOuter->getOp()->getTokenType() == lexer::TokenType::UnionKeyword);
        auto unionInner = dynamic_cast<const BinaryExpression*>(&unionOuter->getLeft());
        REQUIRE(unionInner != nullptr);
        CHECK(unionInner->getOp()->getTokenType() == lexer::TokenType::UnionKeyword);

        ParsedExpression pMask("a MASK b MASK c");
        auto maskOuter = dynamic_cast<BinaryExpression*>(pMask.expression.get());
        REQUIRE(maskOuter != nullptr);
        CHECK(maskOuter->getOp()->getTokenType() == lexer::TokenType::MaskKeyword);
        auto maskInner = dynamic_cast<const BinaryExpression*>(&maskOuter->getLeft());
        REQUIRE(maskInner != nullptr);
        CHECK(maskInner->getOp()->getTokenType() == lexer::TokenType::MaskKeyword);
    }
    SECTION("Bitwise Precedence Relative to PartWord and Arithmetic")
    {
        ParsedExpression p("a + b * BITS [4, 0] c DIFFER d");
        auto addOp = dynamic_cast<BinaryExpression*>(p.expression.get());
        REQUIRE(addOp != nullptr);
        CHECK(addOp->getOp()->getTokenType() == lexer::TokenType::Plus);

        auto mulOp = dynamic_cast<const BinaryExpression*>(&addOp->getRight());
        REQUIRE(mulOp != nullptr);
        CHECK(mulOp->getOp()->getTokenType() == lexer::TokenType::Asterisk);

        auto diffOp = dynamic_cast<const BinaryExpression*>(&mulOp->getRight());
        REQUIRE(diffOp != nullptr);
        CHECK(diffOp->getOp()->getTokenType() == lexer::TokenType::DifferKeyword);
        CHECK(dynamic_cast<const PartWordExpression*>(&diffOp->getLeft()) != nullptr);
        CHECK(dynamic_cast<const IdentifierExpression*>(&diffOp->getRight()) != nullptr);
    }
    SECTION("Bitwise Grouping via INTEGER Type Change")
    {
        ParsedExpression p("a MASK INTEGER(b UNION c)");
        auto maskOp = dynamic_cast<BinaryExpression*>(p.expression.get());
        REQUIRE(maskOp != nullptr);
        CHECK(maskOp->getOp()->getTokenType() == lexer::TokenType::MaskKeyword);
        auto tcOp = dynamic_cast<const TypeChangeExpression*>(&maskOp->getRight());
        REQUIRE(tcOp != nullptr);
        CHECK(tcOp->getType()->getTokenType() == lexer::TokenType::IntegerKeyword);
        auto unionOp = dynamic_cast<const BinaryExpression*>(&tcOp->getExpression());
        REQUIRE(unionOp != nullptr);
        CHECK(unionOp->getOp()->getTokenType() == lexer::TokenType::UnionKeyword);
    }
}

/*
 * 6.1.1.2.1
 */
TEST_CASE("Coral Parse Expression - Word References", "[Parser][Expression]")
{
    SECTION("Array Access Single Index")
    {
        ParsedExpression p("arr [ i ]");
        auto arr = dynamic_cast<ArrayAccessExpression*>(p.expression.get());
        CHECK(arr != nullptr);
        CHECK(arr->getArray() != nullptr);
        CHECK(arr->getIndices().size() == 1);
    }
    SECTION("Array Access Double Index")
    {
        ParsedExpression p("arr [ i, j ]");
        auto arr = dynamic_cast<ArrayAccessExpression*>(p.expression.get());
        CHECK(arr != nullptr);
        CHECK(arr->getArray() != nullptr);
        CHECK(arr->getIndices().size() == 2);
    }
    SECTION("Anonymous Reference")
    {
        ParsedExpression p("[ 10 ]");
        auto arr = dynamic_cast<ArrayAccessExpression*>(p.expression.get());
        CHECK(arr != nullptr);
        CHECK(arr->getArray() == nullptr);
        CHECK(arr->getIndices().size() == 1);
    }
}

/*
 * 6.1.1.2.5
 */
TEST_CASE("Coral Parse Expression - Function Calls", "[Parser][Expression]")
{
    SECTION("No Args Function Call Equivalent (Identifier)")
    {
        const ParsedExpression p("func");
        auto id = dynamic_cast<IdentifierExpression*>(p.expression.get());
        CHECK(id != nullptr);
    }
    SECTION("Function Call with arguments")
    {
        const ParsedExpression p("func(a, b + 1)");
        auto fc = dynamic_cast<FunctionCallExpression*>(p.expression.get());
        CHECK(fc != nullptr);
        CHECK(fc->getArguments().size() == 2);
    }
}

/*
 * 6.1.3
 */
TEST_CASE("Coral Parse Expression - Arithmetic", "[Parser][Expression]")
{
    SECTION("Addition")
    {
        ParsedExpression p("a + b");
        auto binOp = dynamic_cast<BinaryExpression*>(p.expression.get());
        REQUIRE(binOp != nullptr);
        CHECK(binOp->getOp()->getTokenType() == lexer::TokenType::Plus);
    }
    SECTION("Unary Minus")
    {
        ParsedExpression p("-x");
        auto unOp = dynamic_cast<UnaryExpression*>(p.expression.get());
        CHECK(unOp != nullptr);
        CHECK(unOp->getOp()->getTokenType() == lexer::TokenType::Minus);
    }
    SECTION("Implicit Unary Operators")
    {
        ParsedExpression p("a + -b");
        auto binOp = dynamic_cast<BinaryExpression*>(p.expression.get());
        REQUIRE(binOp != nullptr);
        CHECK(binOp->getOp()->getTokenType() == lexer::TokenType::Plus);

        auto right = dynamic_cast<const UnaryExpression*>(&binOp->getRight());
        CHECK(right != nullptr);
        CHECK(right->getOp()->getTokenType() == lexer::TokenType::Minus);
    }
    SECTION("Precedence")
    {
        ParsedExpression p("a + b * c");
        auto binOp = dynamic_cast<BinaryExpression*>(p.expression.get());
        REQUIRE(binOp != nullptr);
        CHECK(binOp->getOp()->getTokenType() == lexer::TokenType::Plus);

        auto right = dynamic_cast<const BinaryExpression*>(&binOp->getRight());
        CHECK(right != nullptr);
        CHECK(right->getOp()->getTokenType() == lexer::TokenType::Asterisk);
    }
}

/*
 * 6.2.1
 */
TEST_CASE("Coral Parse Expression - Conditionals", "[Parser][Expression]")
{
    SECTION("IF THEN ELSE")
    {
        ParsedExpression p("IF a < b THEN c ELSE d");
        auto condExpr = dynamic_cast<ConditionalExpression*>(p.expression.get());
        CHECK(condExpr != nullptr);
        CHECK(dynamic_cast<const BinaryExpression*>(&condExpr->getCondition()) != nullptr);
        CHECK(dynamic_cast<const IdentifierExpression*>(&condExpr->getThenBranch()) != nullptr);
        CHECK(dynamic_cast<const IdentifierExpression*>(&condExpr->getElseBranch()) != nullptr);
        auto thenBranch = dynamic_cast<const IdentifierExpression*>(&condExpr->getThenBranch());
        CHECK(thenBranch != nullptr);
        CHECK(std::get<std::string>(thenBranch->getIdentifier()->getValue()) == "c");
        auto elseBranch = dynamic_cast<const IdentifierExpression*>(&condExpr->getElseBranch());
        CHECK(elseBranch != nullptr);
        CHECK(std::get<std::string>(elseBranch->getIdentifier()->getValue()) == "d");
    }
    SECTION("Nested Conditionals")
    {
        ParsedExpression p("IF a <> 0 THEN IF b = 1 THEN c ELSE d ELSE e");
        auto condExpr = dynamic_cast<ConditionalExpression*>(p.expression.get());
        CHECK(condExpr != nullptr);
    }

    SECTION("Different Comparators")
    {
        ParsedExpression p("IF a >= b AND c <> d OR e <= f THEN x ELSE y");
        auto condExpr = dynamic_cast<ConditionalExpression*>(p.expression.get());
        CHECK(condExpr != nullptr);

        auto orExpr = dynamic_cast<const BinaryExpression*>(&condExpr->getCondition());
        CHECK(orExpr != nullptr);
        CHECK(orExpr->getOp()->getTokenType() == lexer::TokenType::OrKeyword);

        auto andExpr = dynamic_cast<const BinaryExpression*>(&orExpr->getLeft());
        CHECK(andExpr != nullptr);
        CHECK(andExpr->getOp()->getTokenType() == lexer::TokenType::AndKeyword);
    }
}
