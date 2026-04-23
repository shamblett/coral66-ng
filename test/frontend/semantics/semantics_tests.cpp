/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "lib/frontend/semantics/semantics.hpp"

#include "lib/frontend/common/messages/errors.hpp"
#include "lib/frontend/common/source_object.hpp"
#include "lib/frontend/parser/parser.hpp"
#include "test/common/test_config.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

#define semanticsProduces(source, matches)                                   \
    do                                                                       \
    {                                                                        \
        std::string string;                                                  \
        llvm::raw_string_ostream ss(string);                                 \
        coral66::PPSourceObject tokens;                                      \
        bool errors = false;                                                 \
        tokens = coral66::lexer::tokenize(source, &ss, &errors);             \
        REQUIRE_FALSE(errors);                                               \
        auto ctokens = coral66::lexer::toCTokens(tokens, &ss, &errors);      \
        REQUIRE_FALSE(errors);                                               \
        auto tree = coral66::parser::parseProgram(ctokens, &ss, &errors);    \
        REQUIRE_FALSE(errors);                                               \
        coral66::Semantics::analyse(tree, std::move(ctokens), &ss, &errors); \
        CHECK_THAT(string, matches);                                         \
    } while (0)

using namespace coral66::errors::Semantics;

TEST_CASE("Dummy", "[Backend]") {}