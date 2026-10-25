
/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "main.hpp"

#include <llvm/IRPrinter/IRPrintingPasses.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/TargetSelect.h>

#include "action_executor.hpp"
#include "command_line_arguments.hpp"
#include "command_line_processor.hpp"
#include "lib/frontend/common/messages/errors.hpp"
#include "lib/frontend/common/messages/warnings.hpp"
#include "lib/frontend/parser/parser.hpp"
#include "lib/frontend/semantics/program.hpp"
#include "lib/support/filesystem.hpp"
#include "lib/support/scope_exit.hpp"

int coral66::main(const int argc, char** argv, llvm::raw_ostream* const reporter)
{
    // Parse the command line
    CommandLineProcessor::ParseCommandLine(argc, argv);

    // Initialise LLVM
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmPrinters();
    llvm::InitializeAllAsmParsers();

    // Set and Enable colours. default for errors is red, warnings yellow.
    reporter->enable_colors(reporter->has_colors());
    reporter->changeColor(llvm::raw_ostream::RED);
    ScopeExit colorResetter(
        [reporter]
        {
            reporter->resetColor();
            reporter->flush();
        });

    // Initialise the action executor
    auto executor = ActionExecutor(reporter);

    // Check for a source filename with an extension of .cor
    if (cl::SourceFilename.getNumOccurrences() == 0)
    {
        *reporter << errors::CLI::NO_SOURCE_FILES_SPECIFIED.argsCLI();
        return EXIT_FAILURE;
    }
    if (cl::SourceFilename.substr(cl::SourceFilename.find_last_of(".")) != ".cor")
    {
        *reporter << errors::CLI::SOURCE_FILE_EXTENSION_NOT_COR.argsCLI(cl::SourceFilename);
        return EXIT_FAILURE;
    }

    // Check option sanity
    if (cl::CompileOnly)
    {
        if (cl::CompileAndAssembly)
        {
            reporter->changeColor(llvm::raw_ostream::YELLOW);
            *reporter << warnings::CLI::CANNOT_COMPILE_TO_OBJECT_FILE_AND_ASSEMBLY_AT_THE_SAME_TIME.argsCLI();
            reporter->changeColor(llvm::raw_ostream::RED);
        }
        if (cl::PreprocessOutput)
        {
            reporter->changeColor(llvm::raw_ostream::YELLOW);
            *reporter << warnings::CLI::CANNOT_COMPILE_TO_OBJECT_FILE_AND_PREPROCESS_AT_THE_SAME_TIME.argsCLI();
            reporter->changeColor(llvm::raw_ostream::RED);
        }
        return executor.execute(Action::Compile, cl::SourceFilename.data());
    }
    if (cl::CompileAndAssembly)
    {
        if (cl::PreprocessOutput)
        {
            reporter->changeColor(llvm::raw_ostream::YELLOW);
            *reporter << warnings::CLI::CANNOT_COMPILE_TO_ASSEMBLY_AND_PREPROCESS_AT_THE_SAME_TIME.argsCLI();
            reporter->changeColor(llvm::raw_ostream::RED);
        }
        return executor.execute(Action::AssemblyOutput, cl::SourceFilename.data());
    }
    if (cl::PreprocessOutput)
    {
        return executor.execute(Action::Preprocess, cl::SourceFilename.data());
    }
    return executor.execute(Action::Link, cl::SourceFilename.data());
}
