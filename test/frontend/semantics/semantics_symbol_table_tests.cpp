/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 02/05/2026
 */

#include "lib/frontend/semantics/semantics.hpp"

#include "lib/frontend/semantics/symbol_table.hpp"
#include "test/common/test_config.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

using namespace coral66::semantics;
using namespace coral66::errors::semantics;
using namespace coral66::lexer;

TEST_CASE("Construction", "[Semantics][SymbolTable]")
{
    const auto st = SymbolTable();
    CHECK(st.scopeCount() == 1);
}

TEST_CASE("Scopes", "[Semantics][SymbolTable]")
{
    SECTION("Enter/Exit")
    {
        auto st = SymbolTable();
        st.enterScope();
        CHECK(st.scopeCount() == 2);
        st.exitScope();
        CHECK(st.scopeCount() == 1);
        st.exitScope();
        CHECK(st.scopeCount() == 0);
        st.exitScope();
        CHECK(st.scopeCount() == 0);
        st.enterScope();
        CHECK(st.scopeCount() == 1);
    }
}

TEST_CASE("Insert", "[Semantics][SymbolTable]")
{
    SECTION("Current Scope")
    {
        CToken nameToken(TokenType::Identifier, 0, 0, 0, 0, std::string{"myVar"});
        CTokenIterator cti = &nameToken;

        VariableDeclaration declaration(cti, UntypedType{});

        auto st = SymbolTable();
        CHECK(st.insert("myVar", &declaration));
        CHECK(st.lookup("myVar") == &declaration);
    }
    SECTION("Current Scope  - Already Present")
    {
        CToken nameToken(TokenType::Identifier, 0, 0, 0, 0, std::string{"myVar"});
        CTokenIterator cti = &nameToken;

        VariableDeclaration declaration(cti, UntypedType{});

        auto st = SymbolTable();
        CHECK(st.insert("myVar", &declaration));
        CHECK(st.lookup("myVar") == &declaration);

        CHECK_FALSE(st.insert("myVar", &declaration));
    }
}

TEST_CASE("Look Up", "[Semantics][SymbolTable]")
{
    SECTION("Current Scope")
    {
        auto st = SymbolTable();

        CToken nameToken(TokenType::Identifier, 0, 0, 0, 0, std::string{"myVar"});
        CTokenIterator cti = &nameToken;
        VariableDeclaration declaration(cti, UntypedType{});

        CHECK(st.insert("myVar", &declaration));
        CHECK(st.lookupInCurrentScope("myVar") == &declaration);

        st.enterScope();
        CHECK_FALSE(st.lookupInCurrentScope("myVar") == &declaration);
    }

    SECTION("Current Scope - Not Found")
    {
        auto st = SymbolTable();

        CToken nameToken(TokenType::Identifier, 0, 0, 0, 0, std::string{"myVar"});
        CTokenIterator cti = &nameToken;
        VariableDeclaration declaration(cti, UntypedType{});

        CHECK(st.insert("myVar", &declaration));
        CHECK(st.lookupInCurrentScope("myVar1") == nullptr);
    }

    SECTION("All Scopes")
    {
        auto st = SymbolTable();

        CToken nameToken(TokenType::Identifier, 0, 0, 0, 0, std::string{"myVar"});
        CTokenIterator cti = &nameToken;
        VariableDeclaration declaration(cti, UntypedType{});

        CHECK(st.insert("myVar", &declaration));
        CHECK(st.lookup("myVar") == &declaration);

        st.enterScope();
        CHECK(st.lookup("myVar") == &declaration);
    }

    SECTION("All Scopes - Not Found")
    {
        auto st = SymbolTable();

        CToken nameToken(TokenType::Identifier, 0, 0, 0, 0, std::string{"myVar"});
        CTokenIterator cti = &nameToken;
        VariableDeclaration declaration(cti, UntypedType{});

        CHECK(st.insert("myVar", &declaration));
        CHECK(st.lookup("myVar1") == nullptr);
    }
}
