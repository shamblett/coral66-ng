/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 07/03/2026
 */

#include "common.hpp"
#include "lib/frontend/syntax/syntax.hpp"
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
        CHECK(literal != nullptr);
        CHECK(literal->getLiteral()->getTokenType() == lexer::TokenType::Literal);
        CHECK(literal->getLiteral()->getType() == lexer::CToken::Type::Integer);
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
        CHECK(binOp != nullptr);
        CHECK(binOp->getOp()->getTokenType() == lexer::TokenType::MaskKeyword);
    }
    SECTION("UNION")
    {
        ParsedExpression p("a UNION b");
        auto binOp = dynamic_cast<BinaryExpression*>(p.expression.get());
        CHECK(binOp != nullptr);
        CHECK(binOp->getOp()->getTokenType() == lexer::TokenType::UnionKeyword);
    }
    SECTION("DIFFER")
    {
        ParsedExpression p("a DIFFER b");
        auto binOp = dynamic_cast<BinaryExpression*>(p.expression.get());
        CHECK(binOp != nullptr);
        CHECK(binOp->getOp()->getTokenType() == lexer::TokenType::DifferKeyword);
    }
    SECTION("Bitwise Precedence")
    {
        ParsedExpression p("a DIFFER b UNION c MASK d");
        auto diffOp = dynamic_cast<BinaryExpression*>(p.expression.get());
        CHECK(diffOp != nullptr);
        CHECK(diffOp->getOp()->getTokenType() == lexer::TokenType::DifferKeyword);
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
        CHECK(binOp != nullptr);
        CHECK(binOp->getOp()->getTokenType() == lexer::TokenType::Plus);
    }
    SECTION("Unary Minus")
    {
        ParsedExpression p("-x");
        auto unOp = dynamic_cast<UnaryExpression*>(p.expression.get());
        CHECK(unOp != nullptr);
        CHECK(unOp->getOp()->getTokenType() == lexer::TokenType::Minus);
    }
    SECTION("Precedence")
    {
        ParsedExpression p("a + b * c");
        auto binOp = dynamic_cast<BinaryExpression*>(p.expression.get());
        CHECK(binOp != nullptr);
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
