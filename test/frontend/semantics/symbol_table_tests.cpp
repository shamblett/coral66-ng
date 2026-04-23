/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "lib/frontend/semantics/symbol_table.hpp"

#include "lib/frontend/semantics/semantics.hpp"
#include "test/common/test_config.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

using namespace coral66::Semantics;
using namespace coral66::errors::Semantics;
using namespace coral66::lexer;

TEST_CASE("Construction", "[Semantics][SymbolTable]")
{
    auto st = SymbolTable();
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

        VariableDeclaration decl(cti, UntypedType{});

        auto st = SymbolTable();
        CHECK(st.insert("myVar", &decl));
        CHECK(st.lookup("myVar") == &decl);
    }
 SECTION("Current Scope  - Already Present")
    {
        CToken nameToken(TokenType::Identifier, 0, 0, 0, 0, std::string{"myVar"});
        CTokenIterator cti = &nameToken;

        VariableDeclaration decl(cti, UntypedType{});

        auto st = SymbolTable();
        CHECK(st.insert("myVar", &decl));
        CHECK(st.lookup("myVar") == &decl);

        CHECK_FALSE(st.insert("myVar", &decl));
    }
}
