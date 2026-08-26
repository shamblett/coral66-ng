/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 17/02/2026
 */

#include "action_executor.hpp"

#include <llvm/Bitcode/BitcodeWriterPass.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IRPrinter/IRPrintingPasses.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/TargetParser/Triple.h>

#include <cstdlib>

#include "command_line_arguments.hpp"
#include "lib/backend/code_generation.hpp"
#include "lib/frontend/common/messages/errors.hpp"
#include "lib/frontend/common/messages/warnings.hpp"
#include "lib/frontend/parser/parser.hpp"
#include "lib/frontend/preprocessor/preprocessor.hpp"
#include "lib/frontend/semantics/cmi_processor.hpp"
#include "lib/frontend/semantics/program.hpp"
#include "lib/support/filesystem.hpp"
#include "lib/support/vendor/ctrack/include/ctrack.hpp"

int coral66::ActionExecutor::execute(const Action action, const std::string& file)
{
    // Check for no action
    if (cl::NoAction)
    {
        m_reporter->changeColor(llvm::raw_ostream::YELLOW);
        *m_reporter << warnings::CLI::NO_ACTION_SPEFIFIED.argsCLI();
        m_reporter->changeColor(llvm::raw_ostream::RED);
        return EXIT_SUCCESS;
    }
    auto path = fs::path(file);
    const auto objectFile = compileCoral66File(action, path);
    if (!objectFile)
    {
        return EXIT_FAILURE;
    }
    if ((action == Action::Compile || action == Action::Link) && !cl::SyntaxOnly)
    {
        m_linkableFiles.emplace_back(*objectFile);
        for (const auto& obj : cl::ObjectFiles)
        {
            m_linkableFiles.emplace_back(obj);
        }
    }
    if (action == Action::Link && !cl::SyntaxOnly && !cl::ParseOnly &&!cl::EmitLLVM)
    {
        return linkExecutable();
    }
    return EXIT_SUCCESS;
}

std::optional<std::string> coral66::ActionExecutor::compileCoral66File(const Action action, fs::path& sourceFile) const
{
    CTRACK_NAME("Compile");
    fs::ifstream file(sourceFile, std::ios_base::binary | std::ios_base::ate | std::ios_base::in);
    if (!file.is_open())
    {
        *m_reporter << errors::CLI::FAILED_TO_OPEN_SOURCE_FILE.argsCLI(sourceFile.string());
        return {};
    }

    std::streamsize const size = file.tellg();
    file.seekg(0);
    std::string input(size, '\0');
    file.read(input.data(), size);
    file.close();

    sourceFile = fs::absolute(sourceFile);
    sourceFile = sourceFile.lexically_normal();

    bool errors = false;
    auto PPTokens = lexer::tokenize(std::move(input), m_reporter, &errors, sourceFile.string());
    if (errors)
    {
        return {};
    }

    PPTokens = pp::preprocess(std::move(PPTokens), m_reporter, &errors);
    if (errors)
    {
        return {};
    }

    if (action == Action::Preprocess)
    {
        auto reconstruction =
            pp::reconstruct(PPTokens.data().data(), PPTokens.data().data() + PPTokens.data().size(), PPTokens);

        llvm::outs() << reconstruction;
        llvm::outs().flush();
        return fs::path{};
    }

    auto cTokens = coral66::lexer::toCTokens(std::move(PPTokens), m_reporter, &errors);
    if (errors)
    {
        return {};
    }

    syntax::Program tree = parser::parseProgram(cTokens, m_reporter, &errors);
    if (errors)
    {
        return {};
    }
    if (cl::ParseOnly)
    {
        if (cl::Time)
        {
            auto result = ctrack::result_as_string();
            *m_reporter << "\n" << result << "\n";
        }
        return std::optional<fs::path>{std::in_place};
    }

    auto program = semantics::analyse(tree, std::move(cTokens), m_reporter, &errors, cl::LibraryDirectory.getValue());
    if (errors)
    {
        return {};
    }
    if (cl::SyntaxOnly)
    {
        if (cl::Time)
        {
            auto result = ctrack::result_as_string();
            *m_reporter << "\n" << result << "\n";
        }
        return std::optional<fs::path>{std::in_place};
    }

    cgllvm::Options codegenOptions;
    llvm::LLVMContext context;
    llvm::Module module("", context);
    module.setTargetTriple(llvm::Triple(llvm::sys::getDefaultTargetTriple()));
    if (cl::Pie || cl::Pic)
    {
        codegenOptions.reloc = llvm::Reloc::Model::PIC_;
    }

    // Optimisation level
    switch (cl::OptimizationLevel)
    {
        case cl::OptLevel::None: codegenOptions.ol = llvm::CodeGenOpt::parseLevel('0').value(); break;
        case cl::OptLevel::O1: codegenOptions.ol = llvm::CodeGenOpt::parseLevel('1').value(); break;
        case cl::OptLevel::O2: codegenOptions.ol = llvm::CodeGenOpt::parseLevel('2').value(); break;
        case cl::OptLevel::O3: codegenOptions.ol = llvm::CodeGenOpt::parseLevel('3').value(); break;
        default: llvm::CodeGenOpt::parseLevel('0').value(); break;
    }

    // Debug level
    switch (cl::DebugLevel)
    {
        case cl::DebugLevel::NoDebug: codegenOptions.debugEmission = cgllvm::DebugEmission::None; break;
        case cl::DebugLevel::g1: codegenOptions.debugEmission = cgllvm::DebugEmission::Line; break;
        case cl::DebugLevel::g2: codegenOptions.debugEmission = cgllvm::DebugEmission::Default; break;
        case cl::DebugLevel::g3: codegenOptions.debugEmission = cgllvm::DebugEmission::Extended; break;
        default: codegenOptions.debugEmission = cgllvm::DebugEmission::None; break;
    }

    codegenOptions.baremetal = cl::TargetEnv == cl::TargetEnvironment::Baremetal;

    auto targetMachine = coral66::cgllvm::generateLLVM(module, tree, program, codegenOptions);
    if (!targetMachine)
    {
        *m_reporter << errors::CLI::FAILED_TO_GENERATE_TARGET_MACHINE.argsCLI();
        return {};
    }

    if (llvm::verifyModule(module, &llvm::errs()))
    {
        llvm::errs().flush();
        module.print(llvm::outs(), nullptr);
        std::terminate();
    }
    std::string outputFile;
    if (!cl::OutputFilename.empty() && action != Action::Link)
    {
        outputFile = cl::OutputFilename.getValue();
    }
    else
    {
        auto path = sourceFile;
        if (action == Action::AssemblyOutput)
        {
            if (cl::EmitLLVM)
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
            if (cl::EmitLLVM)
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
        *m_reporter << errors::CLI::FAILED_TO_OPEN_FILE_FOR_OUTPUT.argsCLI(outputFile);
        return {};
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

    CTRACK_NAME("Pass Management");
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

    if (cl::EmitLLVM)
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

    os.flush();

    if (cl::Time)
    {
        auto result = ctrack::result_as_string();
        *m_reporter << "\n" << result << "\n";
    }

    if (tree.getIsSegment())
    {
        fs::path cmiPath = outputFile;
        cmiPath.replace_extension("cmi");

        if (auto libName = std::get<std::string>(tree.getName()->getValue());
            !semantics::CMIProcessor::generateCMIFile(libName, program.getContext().exportedSymbols, cmiPath))
        {
            *m_reporter << "Warning: Failed to generate CMI file for library '" << libName << "'\n";
        }
    }

    return fs::path(outputFile);
}

int coral66::ActionExecutor::linkExecutable() const
{
    if (m_linkableFiles.empty())
    {
        return EXIT_FAILURE;
    }

    std::string outName;
    if (!cl::OutputFilename.empty())
    {
        outName = cl::OutputFilename.getValue();
    }
    else
    {
        auto p = m_linkableFiles.front();
        p.replace_extension("");
        outName = p.string();
    }

    std::string command = "gcc";
    for (const auto& file : m_linkableFiles)
    {
        command += " " + file.string();
    }
    command += " -o " + outName;

    if (const int result = std::system(command.c_str()); result != 0)
    {
        *m_reporter << errors::CLI::FAILED_TO_LINK_EXECUTABLE.argsCLI();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
