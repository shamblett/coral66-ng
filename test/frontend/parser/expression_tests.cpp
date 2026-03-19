/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 07/03/2026
 */

#include "common.hpp"
#include "lib/frontend/syntax/syntax.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

using namespace coral66;
using namespace coral66::Syntax;

/*
 * Tests cover section 6 of the specification
 */

/*
 * 6.1.1.1
 */
TEST_CASE("Coral Parse Expression - Primaries", "[parser][expression]")
{
    SECTION("Literals")
    {
        const ParsedExpr p("42");
        REQUIRE(dynamic_cast<LiteralExpression*>(p.expr.get()) != nullptr);
    }
    SECTION("Identifiers")
    {
        const ParsedExpr p("myVar");
        REQUIRE(dynamic_cast<IdentifierExpression*>(p.expr.get()) != nullptr);
    }
    SECTION("Parenthesized Expression")
    {
        const ParsedExpr p("(42)");
        REQUIRE(dynamic_cast<LiteralExpression*>(p.expr.get()) != nullptr);
    }
}

/*
 * 6.1.1.2
 */
TEST_CASE("Coral Parse Expression - Typed Primaries", "[parser][expression]")
{
    SECTION("Type Change")
    {
        ParsedExpr p("INTEGER ( 3.14 )");
        auto tc = dynamic_cast<TypeChangeExpression*>(p.expr.get());
        REQUIRE(tc != nullptr);
        REQUIRE(tc->getType()->getTokenType() == lexer::TokenType::IntegerKeyword);
    }
    SECTION("Type Change FIXED with scale")
    {
        ParsedExpr p("FIXED (10, 2) (3.14)");
        auto tc = dynamic_cast<TypeChangeExpression*>(p.expr.get());
        REQUIRE(tc != nullptr);
        REQUIRE(tc->getType()->getTokenType() == lexer::TokenType::FixedKeyword);
        REQUIRE(tc->getTotalBits() != nullptr);
        REQUIRE(tc->getFractionBits() != nullptr);
    }
    SECTION("Location")
    {
        ParsedExpr p("LOCATION ( x )");
        auto loc = dynamic_cast<LocationExpression*>(p.expr.get());
        REQUIRE(loc != nullptr);
        REQUIRE(dynamic_cast<const IdentifierExpression*>(&loc->getWordReference()) != nullptr);
    }
    SECTION("Part Word")
    {
        ParsedExpr p("BITS [ 8, 4 ] x");
        auto pw = dynamic_cast<PartWordExpression*>(p.expr.get());
        REQUIRE(pw != nullptr);
        REQUIRE(dynamic_cast<const IdentifierExpression*>(&pw->getBase()) != nullptr);
    }
}

/*
 * 6.1.2
 */
TEST_CASE("Coral Parse Expression - Bitwise", "[parser][expression]")
{
    SECTION("MASK")
    {
        ParsedExpr p("a MASK b");
        auto binOp = dynamic_cast<BinaryExpression*>(p.expr.get());
        REQUIRE(binOp != nullptr);
        REQUIRE(binOp->getOp()->getTokenType() == lexer::TokenType::MaskKeyword);
    }
    SECTION("UNION")
    {
        ParsedExpr p("a UNION b");
        auto binOp = dynamic_cast<BinaryExpression*>(p.expr.get());
        REQUIRE(binOp != nullptr);
        REQUIRE(binOp->getOp()->getTokenType() == lexer::TokenType::UnionKeyword);
    }
    SECTION("DIFFER")
    {
        ParsedExpr p("a DIFFER b");
        auto binOp = dynamic_cast<BinaryExpression*>(p.expr.get());
        REQUIRE(binOp != nullptr);
        REQUIRE(binOp->getOp()->getTokenType() == lexer::TokenType::DifferKeyword);
    }
    SECTION("Bitwise Precedence")
    {
        ParsedExpr p("a DIFFER b UNION c MASK d");
        auto diffOp = dynamic_cast<BinaryExpression*>(p.expr.get());
        REQUIRE(diffOp != nullptr);
        REQUIRE(diffOp->getOp()->getTokenType() == lexer::TokenType::DifferKeyword);
    }
}

/*
 * 6.1.1.2.1
 */
TEST_CASE("Coral Parse Expression - Word References", "[parser][expression]")
{
    SECTION("Array Access Single Index")
    {
        ParsedExpr p("arr [ i ]");
        auto arr = dynamic_cast<ArrayAccessExpression*>(p.expr.get());
        REQUIRE(arr != nullptr);
        REQUIRE(arr->getArray() != nullptr);
        REQUIRE(arr->getIndices().size() == 1);
    }
    SECTION("Array Access Double Index")
    {
        ParsedExpr p("arr [ i, j ]");
        auto arr = dynamic_cast<ArrayAccessExpression*>(p.expr.get());
        REQUIRE(arr != nullptr);
        REQUIRE(arr->getArray() != nullptr);
        REQUIRE(arr->getIndices().size() == 2);
    }
    SECTION("Anonymous Reference")
    {
        ParsedExpr p("[ 10 ]");
        auto arr = dynamic_cast<ArrayAccessExpression*>(p.expr.get());
        REQUIRE(arr != nullptr);
        REQUIRE(arr->getArray() == nullptr);
        REQUIRE(arr->getIndices().size() == 1);
    }
}

/*
 * 6.1.1.2.5
 */
TEST_CASE("Coral Parse Expression - Function Calls", "[parser][expression]")
{
    SECTION("No Args Function Call Equivalent (Identifier)")
    {
        ParsedExpr p("func");
        auto id = dynamic_cast<IdentifierExpression*>(p.expr.get());
        REQUIRE(id != nullptr);
    }
    SECTION("Function Call with arguments")
    {
        ParsedExpr p("func(a, b + 1)");
        auto fc = dynamic_cast<FunctionCallExpression*>(p.expr.get());
        REQUIRE(fc != nullptr);
        REQUIRE(fc->getArguments().size() == 2);
    }
}

/*
 * 6.1.3
 */
TEST_CASE("Coral Parse Expression - Arithmetic", "[parser][expression]")
{
    SECTION("Addition")
    {
        ParsedExpr p("a + b");
        auto binOp = dynamic_cast<BinaryExpression*>(p.expr.get());
        REQUIRE(binOp != nullptr);
        REQUIRE(binOp->getOp()->getTokenType() == lexer::TokenType::Plus);
    }
    SECTION("Unary Minus")
    {
        ParsedExpr p("-x");
        auto unOp = dynamic_cast<UnaryExpression*>(p.expr.get());
        REQUIRE(unOp != nullptr);
        REQUIRE(unOp->getOp()->getTokenType() == lexer::TokenType::Minus);
    }
    SECTION("Precedence")
    {
        ParsedExpr p("a + b * c");
        auto binOp = dynamic_cast<BinaryExpression*>(p.expr.get());
        REQUIRE(binOp != nullptr);
        REQUIRE(binOp->getOp()->getTokenType() == lexer::TokenType::Plus);

        auto right = dynamic_cast<const BinaryExpression*>(&binOp->getRight());
        REQUIRE(right != nullptr);
        REQUIRE(right->getOp()->getTokenType() == lexer::TokenType::Asterisk);
    }
}

/*
 * 6.2.1
 */
TEST_CASE("Coral Parse Expression - Conditionals", "[parser][expression]")
{
    SECTION("IF THEN")
    {
        ParsedExpr p("IF a < b THEN");
        auto condExpr = dynamic_cast<ConditionalExpression*>(p.expr.get());
        REQUIRE(condExpr != nullptr);
        REQUIRE(dynamic_cast<const BinaryExpression*>(&condExpr->getCondition()) != nullptr);
        REQUIRE(dynamic_cast<const IdentifierExpression*>(&condExpr->getThenBranch()) != nullptr);
    }
    SECTION("IF THEN ELSE")
    {
        ParsedExpr p("IF a < b THEN c ELSE d");
        auto condExpr = dynamic_cast<ConditionalExpression*>(p.expr.get());
        REQUIRE(condExpr != nullptr);
        REQUIRE(dynamic_cast<const BinaryExpression*>(&condExpr->getCondition()) != nullptr);
        REQUIRE(dynamic_cast<const IdentifierExpression*>(&condExpr->getThenBranch()) != nullptr);
        REQUIRE(dynamic_cast<const IdentifierExpression*>(&condExpr->getElseBranch()) != nullptr);
    }
    SECTION("Nested Conditionals")
    {
        ParsedExpr p("IF a <> 0 THEN IF b = 1 THEN c ELSE d ELSE e");
        auto condExpr = dynamic_cast<ConditionalExpression*>(p.expr.get());
        REQUIRE(condExpr != nullptr);
    }

    SECTION("Different Comparators")
    {
        ParsedExpr p("IF a >= b AND c <> d OR e <= f THEN x ELSE y");
        auto condExpr = dynamic_cast<ConditionalExpression*>(p.expr.get());
        REQUIRE(condExpr != nullptr);

        auto orExpr = dynamic_cast<const BinaryExpression*>(&condExpr->getCondition());
        REQUIRE(orExpr != nullptr);
        REQUIRE(orExpr->getOp()->getTokenType() == lexer::TokenType::OrKeyword);

        auto andExpr = dynamic_cast<const BinaryExpression*>(&orExpr->getLeft());
        REQUIRE(andExpr != nullptr);
        REQUIRE(andExpr->getOp()->getTokenType() == lexer::TokenType::AndKeyword);
    }
}
