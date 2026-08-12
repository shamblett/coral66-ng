/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "program.hpp"

#include "control_flow_pass.hpp"
#include "lib/support/vendor/ctrack/include/ctrack.hpp"
#include "semantic_support.hpp"
#include "symbol_resolution_pass.hpp"
#include "type_checking_pass.hpp"

namespace coral66::semantics
{
Program analyse(const syntax::Program& parseTree, CSourceObject&& cTokens, llvm::raw_ostream* reporter, bool* errors,
                const std::string& libraryPath)
{
    CTRACK_NAME("Semantics");
    DiagnosticCollector::clearDiagnostics();
    SemanticContext context(cTokens, reporter, errors, libraryPath);
    SymbolResolutionPass symPass(context);
    symPass.run(parseTree);

    TypeCheckingPass typePass(context);
    typePass.run(parseTree);

    ControlFlowPass cfPass(context);
    cfPass.run(parseTree);

    return {std::move(cTokens), std::move(context.ownedUsables), std::move(context)};
}
} // namespace coral66::semantics
