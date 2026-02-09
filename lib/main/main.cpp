
/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "main.hpp"

#include <llvm/Bitcode/BitcodeWriterPass.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/Timer.h>

#include "lib/backend/code_generation.hpp"
#include "lib/frontend/common/error_messages.hpp"
#include "lib/frontend/common/language_options.hpp"
#include "lib/frontend/parser/parser.hpp"
#include "lib/frontend/preprocessor/preprocessor.hpp"
#include "lib/frontend/semantics/program.hpp"
#include "lib/main/command_line.hpp"
#include "lib/support/filesystem.hpp"
#include "lib/support/triple.hpp"

CORAL66_CLI_OPT(OUTPUT_FILE, ("-o <file>", "--output=<file>", "--output <file>"), (std::string_view, file))
("Path of the output file");

CORAL66_CLI_OPT(COMPILE_ONLY, ("-c", "--compile"))("Stop after compiling object files");

CORAL66_CLI_OPT(ASSEMBLY_OUTPUT, ("-S", "--assemble"))("Stop after compiling and output assembly files");

CORAL66_CLI_OPT(PREPROCESS_ONLY, ("-E", "--preprocess"))("Preprocess to stdout or output file");

CORAL66_CLI_OPT(SYNTAX_ONLY, ("-fsyntax-only"))("Stop after semantic analysis and produce no output file");

CORAL66_CLI_OPT(EMIT_LLVM, ("-emit-llvm"))("Use LLVM IR instead of machine code and assembly");

CORAL66_CLI_OPT(OPT, ("-O<level>", "-O", "--optimize", "--optimize=<level>"),
                (std::uint8_t, level))("Optimization level");

CORAL66_CLI_OPT(LIBRARY_PATH, ("-L<dir>", "-L <dir>", "--library-directory <dir>", "--library-directory=<dir>"),
                (std::string_view, dir))
("Library directories", coral66::CLIMultiArg::List);

CORAL66_CLI_OPT(PIE, ("-f[no-]PIE"))("Build a position independent executable");

CORAL66_CLI_OPT(PIC, ("-f[no-]PIC"))("Build position independent code");

CORAL66_CLI_OPT(VERSION, ("--version"))("Print version");

CORAL66_CLI_OPT(HELP, ("--help", "-help"))("Display all options");

CORAL66_CLI_OPT(TIME, ("-time"))("Time individual components");

CORAL66_CLI_OPT(G0, ("-g0"))("No debug info");

CORAL66_CLI_OPT(G1, ("-g1", "-gline-tables-only", "-gmlt"))("Lines only");

CORAL66_CLI_OPT(G2, ("-g2", "-g", "--debug"))("Generated debugging info");

CORAL66_CLI_OPT(G3, ("-g3"))("Generated extded debugging info");

namespace
{

enum class Action
{
    Compile,
    Preprocess,
    AssemblyOutput,
    Link [[maybe_unused]]
};

template <class CL>
std::optional<coral66::fs::path> compileCoral66File(Action action, coral66::fs::path cSourceFile, const CL& cli,
                                              llvm::raw_ostream* reporter)
{
    std::optional<llvm::TimerGroup> timer;
    if (cli.template get<TIME>())
    {
        timer.emplace("Compilation", "Time it took for the whole compilation of " + cSourceFile.string());
    }
    coral66::fs::ifstream file(cSourceFile, std::ios_base::binary | std::ios_base::ate | std::ios_base::in);
    if (!file.is_open())
    {
        if (reporter)
        {
            *reporter << coral66::Errors::CLI::FAILED_TO_OPEN_C_SOURCE_FILE_N.argsCLI(cSourceFile.string());
        }
        return {};
    }

    std::streamsize const size = file.tellg();
    file.seekg(0);
    std::string input(size, '\0');
    file.read(input.data(), size);
    file.close();

    cSourceFile = coral66::fs::absolute(cSourceFile);
    cSourceFile = cSourceFile.lexically_normal();

    std::optional<llvm::TimeRegion> ppTokenTimeRegion;
    if (timer)
    {
        std::optional<llvm::Timer> ppTokenTimer;
        ppTokenTimer.emplace("Lexing preprocessor tokens",
                             "Time it took to lex all preprocessor tokens in " + cSourceFile.string(), *timer);
        ppTokenTimeRegion.emplace(*ppTokenTimer);
    }

    bool errors = false;
    auto pptokens = coral66::Lexer::tokenize(std::move(input), reporter, &errors, cSourceFile.string());
    if (errors)
    {
        return {};
    }
    ppTokenTimeRegion.reset();

    std::optional<llvm::TimeRegion> ppTimeRegion;
    if (timer)
    {
        std::optional<llvm::Timer> ppTimer;
        ppTimer.emplace("Preprocessor", "Time it took to preprocess " + cSourceFile.string(), *timer);
        ppTimeRegion.emplace(*ppTimer);
    }

    pptokens = coral66::PP::preprocess(std::move(pptokens), reporter, &errors);
    if (errors)
    {
        return {};
    }
    ppTimeRegion.reset();
    if (action == Action::Preprocess)
    {
        auto reconstruction =
            coral66::PP::reconstruct(pptokens.data().data(), pptokens.data().data() + pptokens.data().size(), pptokens);
        if (!cli.template get<OUTPUT_FILE>())
        {
            llvm::outs() << reconstruction;
            llvm::outs().flush();
            return coral66::fs::path{};
        }

        coral66::fs::ofstream outputFile(coral66::to_string(*cli.template get<OUTPUT_FILE>()), std::ios_base::out);
        if (!outputFile.is_open())
        {
            if (reporter)
            {
                *reporter << coral66::Errors::CLI::FAILED_TO_OPEN_FILE_N_FOR_OUTPUT.argsCLI(
                    *cli.template get<OUTPUT_FILE>());
            }
            return {};
        }
        outputFile << reconstruction;
        outputFile.close();
        return coral66::fs::path(*cli.template get<OUTPUT_FILE>());
    }

    std::optional<llvm::TimeRegion> cTokenTimeRegion;
    if (timer)
    {
        std::optional<llvm::Timer> cTokenTimer;
        cTokenTimer.emplace("C Token", "Time it took turn PP Tokens into C Tokens " + cSourceFile.string(), *timer);
        cTokenTimeRegion.emplace(*cTokenTimer);
    }

    auto ctokens = coral66::Lexer::toCTokens(std::move(pptokens), reporter, &errors);
    if (errors)
    {
        return {};
    }
    cTokenTimeRegion.reset();

    std::optional<llvm::TimeRegion> parserTimeRegion;
    if (timer)
    {
        std::optional<llvm::Timer> parserTimer;
        parserTimer.emplace("Parser", "Time it took to parse " + cSourceFile.string(), *timer);
        parserTimeRegion.emplace(*parserTimer);
    }

    coral66::Syntax::Program tree = coral66::Parser::parseProgram(ctokens, reporter, &errors);
    if (errors)
    {
        return {};
    }
    parserTimeRegion.reset();

    std::optional<llvm::TimeRegion> semanticsTimeRegion;
    if (timer)
    {
        std::optional<llvm::Timer> semanticsTimer;
        semanticsTimer.emplace("Semantics", "Time it took to semantically analyze " + cSourceFile.string(), *timer);
        semanticsTimeRegion.emplace(*semanticsTimer);
    }

    auto program = coral66::Semantics::analyse(tree, std::move(ctokens), reporter, &errors);
    if (errors)
    {
        return {};
    }
    if (cli.template get<SYNTAX_ONLY>())
    {
        return std::optional<coral66::fs::path>{std::in_place};
    }
    semanticsTimeRegion.reset();

    std::optional<llvm::TimeRegion> codegenTimeRegion;
    if (timer)
    {
        std::optional<llvm::Timer> codegenTimer;
        codegenTimer.emplace("Codegen", "Time it took to generate LLVM IR for " + cSourceFile.string(), *timer);
        codegenTimeRegion.emplace(*codegenTimer);
    }

    coral66::CGLLVM::Options codegenOptions;
    llvm::LLVMContext context;
    llvm::Module module("", context);
    if (cli.template get<PIE>() || cli.template get<PIC>())
    {
        codegenOptions.reloc = llvm::Reloc::Model::PIC_;
    }
    if (cli.template get<OPT>())
    {
        // ReSharper disable once CppDefaultCaseNotHandledInSwitchStatement
        switch (cli.template get<OPT>()->value_or(1))
        {
            case 0: codegenOptions.ol = llvm::CodeGenOpt::parseLevel('0').value(); break;
            case 1: codegenOptions.ol = llvm::CodeGenOpt::parseLevel('1').value(); break;
            case 2: codegenOptions.ol = llvm::CodeGenOpt::parseLevel('2').value(); break;
            default: codegenOptions.ol = llvm::CodeGenOpt::parseLevel('3').value(); break;
        }
    }
    auto* debugOption = cli.template lastSpecified<G0, G1, G2, G3>();
    if (!debugOption)
    {
        debugOption = &G0;
    }
    if (debugOption == &G0)
    {
        codegenOptions.debugEmission = coral66::CGLLVM::DebugEmission::None;
    }
    else if (debugOption == &G1)
    {
        codegenOptions.debugEmission = coral66::CGLLVM::DebugEmission::Line;
    }
    else if (debugOption == &G2)
    {
        codegenOptions.debugEmission = coral66::CGLLVM::DebugEmission::Default;
    }
    else if (debugOption == &G3)
    {
        codegenOptions.debugEmission = coral66::CGLLVM::DebugEmission::Extended;
    }

    auto targetMachine = coral66::CGLLVM::generateLLVM(module, program, codegenOptions);
    codegenTimeRegion.reset();
#ifndef NDEBUG
    if (llvm::verifyModule(module, &llvm::errs()))
    {
        llvm::errs().flush();
        module.print(llvm::outs(), nullptr);
        std::terminate();
    }
#endif
    std::string outputFile;
    if (cli.template get<OUTPUT_FILE>())
    {
        outputFile = *cli.template get<OUTPUT_FILE>();
    }
    else
    {
        auto path = cSourceFile;
        if (action == Action::AssemblyOutput)
        {
            if (cli.template get<EMIT_LLVM>())
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
            if (cli.template get<EMIT_LLVM>())
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
            *reporter << coral66::Errors::CLI::FAILED_TO_OPEN_FILE_N_FOR_OUTPUT.argsCLI(outputFile);
        }
        return {};
    }

    std::optional<llvm::TimeRegion> compileTimeRegion;
    if (timer)
    {
        std::optional<llvm::Timer> compileTimer;
        compileTimer.emplace("Compile", "Time it took for LLVM to generate native object code " + cSourceFile.string(),
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

    if (cli.template get<EMIT_LLVM>())
    {
        if (action == Action::AssemblyOutput)
        {
            // modulePassManager.addPass(llvm::PrintModulePass(os));
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

template <class CL>
int doActionOnAllFiles(Action action, std::span<const std::string_view> files, const CL& cli,
                       llvm::raw_ostream* reporter)
{
    std::vector<coral66::fs::path> linkableFiles;
    for (auto& iter : files)
    {
        auto path = coral66::fs::path(iter);
        auto extension = path.extension();
        if (extension == ".c")
        {
            auto objectFile = compileCoral66File(action, iter, cli, reporter);
            if (!objectFile)
            {
                return -1;
            }
            if (action == Action::Compile && !cli.template get<SYNTAX_ONLY>())
            {
                linkableFiles.push_back(*objectFile);
            }
        }
    }
    return 0;
}

void printVersion(llvm::raw_ostream& os) // NOLINT(*-prefer-static-over-anonymous-namespace)
{
    os << "coral66 version " << "1.0-beta" << '\n';
    os.flush();
}

} // namespace

int coral66::main(std::span<std::string_view> elements, llvm::raw_ostream* const reporter, llvm::raw_ostream* out)
{
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmPrinters();
    llvm::InitializeAllAsmParsers();

    auto cli =
        coral66::parseCommandLine<OUTPUT_FILE, COMPILE_ONLY, ASSEMBLY_OUTPUT, PREPROCESS_ONLY, EMIT_LLVM, OPT,
                                  LIBRARY_PATH, PIE, PIC, HELP, VERSION, G0, G1, G2, G3, SYNTAX_ONLY, TIME>(elements);
    if (cli.get<HELP>())
    {
        if (out)
        {
            cli.printHelp(*out);
        }
        return 0;
    }
    if (cli.get<VERSION>())
    {
        if (out)
        {
            printVersion(*out);
        }
        return 0;
    }

    if (cli.getUnrecognized().empty())
    {
        if (reporter)
        {
            *reporter << Errors::CLI::NO_SOURCE_FILES_SPECIFIED.argsCLI();
        }
        return -1;
    }
    if (cli.get<COMPILE_ONLY>())
    {
        if (cli.get<ASSEMBLY_OUTPUT>())
        {
            if (reporter)
            {
                *reporter << Errors::CLI::CANNOT_COMPILE_TO_OBJECT_FILE_AND_ASSEMBLY_AT_THE_SAME_TIME.argsCLI();
            }
        }
        if (cli.get<PREPROCESS_ONLY>())
        {
            if (reporter)
            {
                *reporter << Errors::CLI::CANNOT_COMPILE_TO_OBJECT_FILE_AND_PREPROCESS_AT_THE_SAME_TIME.argsCLI();
            }
        }
        return doActionOnAllFiles(Action::Compile, cli.getUnrecognized(), cli, reporter);
    }
    if (cli.get<ASSEMBLY_OUTPUT>())
    {
        if (cli.get<PREPROCESS_ONLY>())
        {
            if (reporter)
            {
                *reporter << Errors::CLI::CANNOT_COMPILE_TO_ASSEMBLY_AND_PREPROCESS_AT_THE_SAME_TIME.argsCLI();
            }
        }
        return doActionOnAllFiles(Action::AssemblyOutput, cli.getUnrecognized(), cli, reporter);
    }
    if (cli.get<PREPROCESS_ONLY>())
    {
        return doActionOnAllFiles(Action::Preprocess, cli.getUnrecognized(), cli, reporter);
    }
    return doActionOnAllFiles(Action::Compile, cli.getUnrecognized(), cli, reporter);
}
