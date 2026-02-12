
/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "main.hpp"

#include <llvm/Bitcode/BitcodeWriterPass.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IRPrinter/IRPrintingPasses.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/Timer.h>

#include <iostream>

#include "command_line_arguments.hpp"
#include "command_line_processor.hpp"
#include "lib/backend/code_generation.hpp"
#include "lib/frontend/common/error_messages.hpp"
#include "lib/frontend/parser/parser.hpp"
#include "lib/frontend/preprocessor/preprocessor.hpp"
#include "lib/frontend/semantics/program.hpp"
#include "lib/support/filesystem.hpp"

namespace
{

enum class Action
{
    Compile,
    Preprocess,
    AssemblyOutput,
    Link [[maybe_unused]]
};

std::optional<std::string> compileCoral66File(Action action, coral66::fs::path sourceFile, llvm::raw_ostream* reporter)
{
    std::optional<llvm::TimerGroup> timer;
    if (coral66::cl::Time)
    {
        timer.emplace("Compilation", "Time it took for the whole compilation of " + sourceFile.string());
    }
    coral66::fs::ifstream file(sourceFile, std::ios_base::binary | std::ios_base::ate | std::ios_base::in);
    if (!file.is_open())
    {
        if (reporter)
        {
            *reporter << coral66::Errors::CLI::FAILED_TO_OPEN_SOURCE_FILE.argsCLI(sourceFile.string());
        }
        return {};
    }

    std::streamsize const size = file.tellg();
    file.seekg(0);
    std::string input(size, '\0');
    file.read(input.data(), size);
    file.close();

    sourceFile = coral66::fs::absolute(sourceFile);
    sourceFile = sourceFile.lexically_normal();

    std::optional<llvm::TimeRegion> ppTokenTimeRegion;
    if (timer)
    {
        std::optional<llvm::Timer> ppTokenTimer;
        ppTokenTimer.emplace("Lexing preprocessor tokens",
                             "Time it took to lex all preprocessor tokens in " + sourceFile.string(), *timer);
        ppTokenTimeRegion.emplace(*ppTokenTimer);
    }

    bool errors = false;
    auto ppTokens = coral66::Lexer::tokenize(std::move(input), reporter, &errors, sourceFile.string());
    if (errors)
    {
        return {};
    }
    ppTokenTimeRegion.reset();

    std::optional<llvm::TimeRegion> ppTimeRegion;
    if (timer)
    {
        std::optional<llvm::Timer> ppTimer;
        ppTimer.emplace("Preprocessor", "Time it took to preprocess " + sourceFile.string(), *timer);
        ppTimeRegion.emplace(*ppTimer);
    }

    ppTokens = coral66::PP::preprocess(std::move(ppTokens), reporter, &errors);
    if (errors)
    {
        return {};
    }
    ppTimeRegion.reset();
    if (action == Action::Preprocess)
    {
        auto reconstruction =
            coral66::PP::reconstruct(ppTokens.data().data(), ppTokens.data().data() + ppTokens.data().size(), ppTokens);
        if (!coral66::cl::OutputFilename.empty())
        {
            llvm::outs() << reconstruction;
            llvm::outs().flush();
            return coral66::fs::path{};
        }

        coral66::fs::ofstream outputFile(coral66::to_string(coral66::cl::OutputFilename), std::ios_base::out);
        if (!outputFile.is_open())
        {
            if (reporter)
            {
                *reporter << coral66::Errors::CLI::FAILED_TO_OPEN_FILE_FOR_OUTPUT.argsCLI(
                    coral66::cl::OutputFilename);
            }
            return {};
        }
        outputFile << reconstruction;
        outputFile.close();
        return coral66::cl::OutputFilename;
    }

    std::optional<llvm::TimeRegion> cTokenTimeRegion;
    if (timer)
    {
        std::optional<llvm::Timer> cTokenTimer;
        cTokenTimer.emplace("C Token", "Time it took turn PP Tokens into C Tokens " + sourceFile.string(), *timer);
        cTokenTimeRegion.emplace(*cTokenTimer);
    }

    auto cTokens = coral66::Lexer::toCTokens(std::move(ppTokens), reporter, &errors);
    if (errors)
    {
        return {};
    }
    cTokenTimeRegion.reset();

    std::optional<llvm::TimeRegion> parserTimeRegion;
    if (timer)
    {
        std::optional<llvm::Timer> parserTimer;
        parserTimer.emplace("Parser", "Time it took to parse " + sourceFile.string(), *timer);
        parserTimeRegion.emplace(*parserTimer);
    }

    coral66::Syntax::Program tree = coral66::Parser::parseProgram(cTokens, reporter, &errors);
    if (errors)
    {
        return {};
    }
    parserTimeRegion.reset();

    std::optional<llvm::TimeRegion> semanticsTimeRegion;
    if (timer)
    {
        std::optional<llvm::Timer> semanticsTimer;
        semanticsTimer.emplace("Semantics", "Time it took to semantically analyze " + sourceFile.string(), *timer);
        semanticsTimeRegion.emplace(*semanticsTimer);
    }

    auto program = coral66::Semantics::analyse(tree, std::move(cTokens), reporter, &errors);
    if (errors)
    {
        return {};
    }
    if (coral66::cl::SyntaxOnly)
    {
        return std::optional<coral66::fs::path>{std::in_place};
    }
    semanticsTimeRegion.reset();

    std::optional<llvm::TimeRegion> codegenTimeRegion;
    if (timer)
    {
        std::optional<llvm::Timer> codegenTimer;
        codegenTimer.emplace("Codegen", "Time it took to generate LLVM IR for " + sourceFile.string(), *timer);
        codegenTimeRegion.emplace(*codegenTimer);
    }

    coral66::CGLLVM::Options codegenOptions;
    llvm::LLVMContext context;
    llvm::Module module("", context);
    if (coral66::cl::Pie || coral66::cl::Pic)
    {
        codegenOptions.reloc = llvm::Reloc::Model::PIC_;
    }

    // Optimisation level
    switch (coral66::cl::OptimizationLevel)
    {
        case coral66::cl::OptLevel::None: codegenOptions.ol = llvm::CodeGenOpt::parseLevel('0').value(); break;
        case coral66::cl::OptLevel::O1: codegenOptions.ol = llvm::CodeGenOpt::parseLevel('1').value(); break;
        case coral66::cl::OptLevel::O2: codegenOptions.ol = llvm::CodeGenOpt::parseLevel('2').value(); break;
        case coral66::cl::OptLevel::O3: codegenOptions.ol = llvm::CodeGenOpt::parseLevel('3').value(); break;
        default:
            llvm::CodeGenOpt::parseLevel('0').value();
            break;
            ;
    }

    // Debug level
    switch (coral66::cl::DebugLevel)
    {
        case coral66::cl::DebugLevel::NoDebug:
            codegenOptions.debugEmission = coral66::CGLLVM::DebugEmission::None;
            break;
        case coral66::cl::DebugLevel::g1: codegenOptions.debugEmission = coral66::CGLLVM::DebugEmission::Line; break;
        case coral66::cl::DebugLevel::g2: codegenOptions.debugEmission = coral66::CGLLVM::DebugEmission::Default; break;
        case coral66::cl::DebugLevel::g3:
            codegenOptions.debugEmission = coral66::CGLLVM::DebugEmission::Extended;
            break;
        default:
            codegenOptions.debugEmission = coral66::CGLLVM::DebugEmission::None;
            break;
            ;
    }

    auto targetMachine = coral66::CGLLVM::generateLLVM(module, program, codegenOptions);
    codegenTimeRegion.reset();
    if (llvm::verifyModule(module, &llvm::errs()))
    {
        llvm::errs().flush();
        module.print(llvm::outs(), nullptr);
        std::terminate();
    }
    std::string outputFile;
    if (!coral66::cl::OutputFilename.empty())
    {
        outputFile = coral66::cl::OutputFilename.getValue();
    }
    else
    {
        auto path = sourceFile;
        if (action == Action::AssemblyOutput)
        {
            if (coral66::cl::EmitLLVM)
            {
                path.replace_extension("ll");
            }
            else
            {
                path.replace_extension("s");
            }
        }
        else
        {
            if (coral66::cl::EmitLLVM)
            {
                path.replace_extension("bc");
            }
            else
            {
                path.replace_extension("o");
            }
        }
        outputFile = path.string();
    }

    std::error_code ec;
    llvm::raw_fd_ostream os(outputFile, ec, llvm::sys::fs::OpenFlags::OF_None);
    if (ec)
    {
        if (reporter)
        {
            *reporter << coral66::Errors::CLI::FAILED_TO_OPEN_FILE_FOR_OUTPUT.argsCLI(outputFile);
        }
        return {};
    }

    std::optional<llvm::TimeRegion> compileTimeRegion;
    if (timer)
    {
        std::optional<llvm::Timer> compileTimer;
        compileTimer.emplace("Compile", "Time it took for LLVM to generate native object code " + sourceFile.string(),
                             *timer);
        compileTimeRegion.emplace(*compileTimer);
    }
    llvm::PassBuilder passBuilder(targetMachine.get());
    llvm::LoopAnalysisManager loopAnalysisManager;
    llvm::FunctionAnalysisManager functionAnalysisManager;
    llvm::CGSCCAnalysisManager cgsccAnalysisManager;
    llvm::ModuleAnalysisManager moduleAnalysisManager;

    passBuilder.registerModuleAnalyses(moduleAnalysisManager);
    passBuilder.registerCGSCCAnalyses(cgsccAnalysisManager);
    passBuilder.registerFunctionAnalyses(functionAnalysisManager);
    passBuilder.registerLoopAnalyses(loopAnalysisManager);
    passBuilder.crossRegisterProxies(loopAnalysisManager, functionAnalysisManager, cgsccAnalysisManager,
                                     moduleAnalysisManager);

    llvm::ModulePassManager modulePassManager;
    if (codegenOptions.ol == llvm::CodeGenOptLevel::None)
    {
        modulePassManager = passBuilder.buildO0DefaultPipeline(llvm::OptimizationLevel::O0);
    }
    else
    {
        llvm::OptimizationLevel level = llvm::OptimizationLevel::O2;
        switch (codegenOptions.ol)
        {
            case llvm::CodeGenOptLevel::Less: level = llvm::OptimizationLevel::O1; break;
            case llvm::CodeGenOptLevel::Default: level = llvm::OptimizationLevel::O2; break;
            case llvm::CodeGenOptLevel::Aggressive: level = llvm::OptimizationLevel::O3; break;
            default: break;
        }
        modulePassManager = passBuilder.buildPerModuleDefaultPipeline(level);
    }

    if (coral66::cl::EmitLLVM)
    {
        if (action == Action::AssemblyOutput)
        {
            modulePassManager.addPass(llvm::PrintModulePass(os));
        }
        else
        {
            modulePassManager.addPass(llvm::BitcodeWriterPass(os));
        }
        modulePassManager.run(module, moduleAnalysisManager);
    }
    else
    {
        modulePassManager.run(module, moduleAnalysisManager);
        llvm::legacy::PassManager codegenPass;
        if (targetMachine->addPassesToEmitFile(codegenPass, os, nullptr,
                                               action == Action::AssemblyOutput ? llvm::CodeGenFileType::AssemblyFile :
                                                                                  llvm::CodeGenFileType::ObjectFile))
        {
            return {};
        }
        codegenPass.run(module);
    }

    compileTimeRegion.reset();
    os.flush();

    return coral66::fs::path(outputFile);
}

int performAction(const Action action, const std::string& file, llvm::raw_ostream* reporter)
{
    // Check for no action
    if ( coral66::cl::NoAction)
    {
        return EXIT_SUCCESS;
    }
    auto path = coral66::fs::path(file);
    const auto objectFile = compileCoral66File(action, file, reporter);
    if (!objectFile)
    {
        return EXIT_FAILURE;
    }
    if (action == Action::Compile && !coral66::cl::SyntaxOnly)
    {
        std::vector<coral66::fs::path> linkableFiles;
        linkableFiles.emplace_back(*objectFile);
    }
    return 0;
}

} // namespace

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
    reporter->changeColor(llvm::raw_ostream::RED);
    reporter->enable_colors(true);

    // Check for a source filename with an extension of .cor
    if (cl::SourceFilename.empty())
    {
        *reporter << Errors::CLI::NO_SOURCE_FILES_SPECIFIED.argsCLI();
        return EXIT_FAILURE;
    }
    if (cl::SourceFilename.substr(cl::SourceFilename.find_last_of(".") + 1) != ".cor")
    {
        *reporter << Errors::CLI::SOURCE_FILE_EXTENSION_NOT_COR.argsCLI(cl::SourceFilename);
        return EXIT_FAILURE;
    }

    // Check option sanity
    if (cl::CompileOnly)
    {
        if (cl::CompileAndAssembly)
        {
            reporter->changeColor(llvm::raw_ostream::YELLOW);
            *reporter << Errors::CLI::CANNOT_COMPILE_TO_OBJECT_FILE_AND_ASSEMBLY_AT_THE_SAME_TIME.argsCLI();
            reporter->changeColor(llvm::raw_ostream::RED);
        }
        if (cl::PreprocessOutput)
        {
            reporter->changeColor(llvm::raw_ostream::YELLOW);
            *reporter << Errors::CLI::CANNOT_COMPILE_TO_OBJECT_FILE_AND_PREPROCESS_AT_THE_SAME_TIME.argsCLI();
            reporter->changeColor(llvm::raw_ostream::RED);
        }
        return performAction(Action::Compile, cl::SourceFilename, reporter);
    }
    if (cl::CompileAndAssembly)
    {
        if (cl::PreprocessOutput)
        {
            reporter->changeColor(llvm::raw_ostream::YELLOW);
            *reporter << Errors::CLI::CANNOT_COMPILE_TO_ASSEMBLY_AND_PREPROCESS_AT_THE_SAME_TIME.argsCLI();
            reporter->changeColor(llvm::raw_ostream::RED);
        }
        return performAction(Action::AssemblyOutput, cl::SourceFilename, reporter);
    }
    if (cl::PreprocessOutput)
    {
        return performAction(Action::Preprocess, cl::SourceFilename, reporter);
    }
    return performAction(Action::Compile, cl::SourceFilename, reporter);
}
