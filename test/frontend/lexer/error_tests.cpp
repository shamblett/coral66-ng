/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include <llvm/Support/Format.h>


#include "lib/frontend/common/messages/errors.hpp"
#include "lib/frontend/common/source_object.hpp"
#include "lib/frontend/lexer/lexer.hpp"
#include "test/common/test_config.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

/*
 *
 * Tests cover section 10 and Appendix 2 of the specification
 * correct error generation
 *
 */

using namespace coral66::errors::lexer;
using namespace Catch::Matchers;

using namespace coral66::lexer;

namespace
{

coral66::PPSourceObject lex(const std::string& code, bool& errors)
{
    std::string buffer;
    llvm::raw_string_ostream ss(buffer);
    return tokenize(code, &ss, &errors);
}

} // namespace

TEST_CASE("Coral Lexing Literals - Errors", "[lexer]")
{
    SECTION("Unterminated String Literal")
    {
        bool errors = false;
        lex("'Hello World", errors);
        CHECK(errors);
    }
}
