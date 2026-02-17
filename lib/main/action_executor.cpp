/*
* Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 17/02/2026
 */

#include "action_executor.hpp"

#include "command_line_arguments.hpp"

#include "lib/frontend/common/messages/warnings.hpp"

int coral66::ActionExecutor::execute(const Action action, const std::string& file)
{

    // Check for no action
    if (coral66::cl::NoAction)
    {
        m_reporter->changeColor(llvm::raw_ostream::YELLOW);
        *m_reporter << coral66::Warnings::CLI::NO_ACTION_SPEFIFIED.argsCLI();
        m_reporter->changeColor(llvm::raw_ostream::RED);
        return EXIT_SUCCESS;
    }
    auto path = fs::path(file);
    const auto objectFile = compileCoral66File(action, file);
    if (!objectFile)
    {
        return EXIT_FAILURE;
    }
    if (action == Action::Compile && !coral66::cl::SyntaxOnly)
    {
        m_linkableFiles.emplace_back(*objectFile);
    }
    return EXIT_SUCCESS;
}

std::optional<std::string> coral66::ActionExecutor::compileCoral66File(const Action action, const fs::path& sourceFile) {}
