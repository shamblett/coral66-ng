
/*
* Project: coral66
* Author : Copyright S. Hamblett <steve.hamblett@linux.com>
* Date   : 31/01/2026
*/

#include "Main.hpp"

#include <llvm/Bitcode/BitcodeWriterPass.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/Timer.h>

#include "frontend/common/ErrorMessages.hpp"
#include "frontend/common/LanguageOptions.hpp"
#include "frontend/compiler/Parser.hpp"
#include "frontend/compiler/Program.hpp"
#include "frontend/compiler/Preprocessor.hpp"
#include "backend/CodeGeneration.hpp"
#include "support/Filesystem.hpp"
#include "support/Triple.hpp"
#include "main/CommandLine.hpp"

CLD_CLI_OPT(OUTPUT_FILE, ("-o <file>", "--output=<file>", "--output <file>"), (std::string_view, file))
("Path of the output file");

CLD_CLI_OPT(COMPILE_ONLY, ("-c", "--compile"))("Stop after compiling object files");

CLD_CLI_OPT(ASSEMBLY_OUTPUT, ("-S", "--assemble"))("Stop after compiling and output assembly files");

CLD_CLI_OPT(PREPROCESS_ONLY, ("-E", "--preprocess"))("Preprocess to stdout or output file");

CLD_CLI_OPT(SYNTAX_ONLY, ("-fsyntax-only"))("Stop after semantic analysis and produce no output file");

CLD_CLI_OPT(TARGET, ("--target=<arg>", "-target <arg>"), (std::string_view, arg))("Compiler target triple to use");

CLD_CLI_OPT(DEFINE_MACRO, ("-D<macro>=<value>", "-D<macro>", "--define-macro <macro>", "--define-macro=<macro>"),
            (std::string_view, macro), (std::string_view, value))
("Define macro for the whole translation unit", coral66::CLIMultiArg::List);

CLD_CLI_OPT(EMIT_LLVM, ("-emit-llvm"))("Use LLVM IR instead of machine code and assembly");

CLD_CLI_OPT(OPT, ("-O<level>", "-O", "--optimize", "--optimize=<level>"), (std::uint8_t, level))("Optimization level");

CLD_CLI_OPT(INCLUDES, ("-I<dir>", "-I <dir>", "--include-directory <dir>", "--include-directory=<dir>"),
            (std::string_view, dir))
("Additional include directories", coral66::CLIMultiArg::List);

CLD_CLI_OPT(ISYSTEM, ("-isystem<dir>", "-isystem <dir>"), (std::string, dir))
("Append to system include directories", coral66::CLIMultiArg::List);

CLD_CLI_OPT(PIE, ("-f[no-]PIE"))("Build a position independent executable");

CLD_CLI_OPT(PIC, ("-f[no-]PIC"))("Build position independent code");

CLD_CLI_OPT(FREESTANDING, ("-ffreestanding"))("Compile in a freestanding environment");

CLD_CLI_OPT(EMIT_ALL_DECLS, ("-femit-all-decls"))("Compile in a freestanding environment");

CLD_CLI_OPT(VERSION, ("--version"))("Print version");

CLD_CLI_OPT(HELP, ("--help", "-help"))("Display all options");

CLD_CLI_OPT(TIME, ("-time"))("Time individual components");

CLD_CLI_OPT(STANDARD_VERSION, ("-std=<arg>", "--std=<arg>", "--std <arg>"), (std::string_view, arg))
("C standard version");

CLD_CLI_OPT(WARNINGS, ("-W[no-]<warning>", "--warn-[no-]<warning>", "--warn-=[no-]<warning>"),
            (std::string_view, warning))
("Enable/disable specified warning", coral66::CLIMultiArg::List);

CLD_CLI_OPT(G0, ("-g0"))("No debug info");

CLD_CLI_OPT(G1, ("-g1", "-gline-tables-only", "-gmlt"))("Lines only");

CLD_CLI_OPT(G2, ("-g2", "-g", "--debug"))("Generated debugging info");

CLD_CLI_OPT(G3, ("-g3"))("Generated extded debugging info");

CLD_CLI_OPT(MMX, ("-m[no-]mx"))("Enable x86 MMX extensions");

CLD_CLI_OPT(SSE, ("-m[no-]sse"))("Enable x86 SSE extensions");

CLD_CLI_OPT(THREEDNOWA, ("-m[no-]3dnowa"))("Enable x86 3DNowA! extensions");

CLD_CLI_OPT(SSE2, ("-m[no-]sse2"))("Enable x86 SSE 2 extensions");

CLD_CLI_OPT(SSE3, ("-m[no-]sse3"))("Enable x86 SSE 3 extensions");

CLD_CLI_OPT(SSSE3, ("-m[no-]ssse3"))("Enable x86 SSSE 3 extensions");

CLD_CLI_OPT(SSE4_1, ("-m[no-]sse4.1"))("Enable x86 SSE 4.1 extensions");

CLD_CLI_OPT(SSE4_2, ("-m[no-]sse4.2"))("Enable x86 SSE 4.2 extensions");

CLD_CLI_OPT(AVX, ("-m[no-]avx"))("Enable x86 AVX extensions");

CLD_CLI_OPT(AVX2, ("-m[no-]avx2"))("Enable x86 AVX 2 extensions");

CLD_CLI_OPT(AES, ("-m[no-]aes"))("Enable x86 AES extensions");

CLD_CLI_OPT(PCLMUL, ("-m[no-]pclmul"))("Enable x86 PCLMUL extensions");

CLD_CLI_OPT(FSGSBASE, ("-m[no-]fsgsbase"))("Enable x86 FSGSBASE extensions");

CLD_CLI_OPT(RDRND, ("-m[no-]frdrnd"))("Enable x86 RDRND extensions");

CLD_CLI_OPT(PTWRITE, ("-m[no-]ptwrite"))("Enable x86 PTWRITE extensions");

CLD_CLI_OPT(SSE4A, ("-m[no-]sse4a"))("Enable x86 SSE 4a extensions");

CLD_CLI_OPT(XOPA, ("-m[no-]xop"))("Enable x86 XOP extensions");

CLD_CLI_OPT(FMA4, ("-m[no-]fma4"))("Enable x86 FMA4 extensions");

CLD_CLI_OPT(LWP, ("-m[no-]lwp"))("Enable x86 LWP extensions");

CLD_CLI_OPT(BMI, ("-m[no-]bmi"))("Enable x86 BMI extensions");

CLD_CLI_OPT(BMI2, ("-m[no-]bmi2"))("Enable x86 BMI 2 extensions");

CLD_CLI_OPT(LZCNT, ("-m[no-]lzcnt"))("Enable x86 LZCNT extensions");

CLD_CLI_OPT(FXSR, ("-m[no-]fxsr"))("Enable x86 FXSR extensions");

CLD_CLI_OPT(XSAVE, ("-m[no-]xsave"))("Enable x86 XSAVE extensions");

CLD_CLI_OPT(XSAVEOPT, ("-m[no-]xsaveopt"))("Enable x86 XSAVEOPT extensions");

CLD_CLI_OPT(TBM, ("-m[no-]tbm"))("Enable x86 TBM extensions");

CLD_CLI_OPT(THREEDNOW, ("-m[no-]3dnow"))("Enable x86 3DNow! extensions");

CLD_CLI_OPT(RTM, ("-m[no-]rtm"))("Enable x86 RTM extensions");

CLD_CLI_OPT(MWAITX, ("-m[no-]mwaitx"))("Enable x86 MWAITX extensions");

CLD_CLI_OPT(CLZERO, ("-m[no-]mwaitx"))("Enable x86 CLZERO extensions");

CLD_CLI_OPT(PKU, ("-m[no-]pku"))("Enable x86 PKU extensions");

 // ReSharper disable once CppRedundantNamespaceDefinition
namespace coral66::cli
{
template <>
struct InitialStorage<WARNINGS>
{
    static auto getInitial()
    {
        return coral66::diag::getAllWarnings();
    }
};
}; // namespace coral66::cli

namespace
{
template <class CL>
void addUnixMacroStyle(coral66::PP::Options& ppOptions, const coral66::LanguageOptions& languageOptions, std::string_view const name, // NOLINT(*-prefer-static-over-anonymous-namespace)
                       const CL&)
{
    auto alloc = coral66::to_string(name);
    ppOptions.additionalMacros.emplace_back("__" + alloc, "1");
    ppOptions.additionalMacros.emplace_back("__" + alloc + "__", "1");
    if (languageOptions.extension == coral66::LanguageOptions::Extension::GNU)
    {
        ppOptions.additionalMacros.emplace_back(alloc, "1");
    }
}

template <class Integer>
 // ReSharper disable once CppDFAUnreachableFunctionCall
Integer safeShiftLeft(Integer number, std::size_t value) // NOLINT(*-prefer-static-over-anonymous-namespace)
{
    if (value >= std::numeric_limits<Integer>::digits)
    {
        return 0;
    }

    return number << value;
}

template <class CL>
coral66::PP::Options getTargetSpecificPreprocessorOptions(const coral66::LanguageOptions& languageOptions, // NOLINT(*-prefer-static-over-anonymous-namespace)
                                                      const coral66::Triple& triple, const CL& cli)
{
    coral66::PP::Options result;
    if (triple.getArchitecture() == coral66::Architecture::x86_64)
    {
        result.additionalMacros.emplace_back("__amd64__", "1");
        result.additionalMacros.emplace_back("__amd64", "1");
        result.additionalMacros.emplace_back("__x86_64", "1");
        result.additionalMacros.emplace_back("__x86_64__", "1");
        result.additionalMacros.emplace_back("__NO_MATH_INLINES", "1");
    }
    if (triple.getPlatform() == coral66::Platform::Linux)
    {
        addUnixMacroStyle(result, languageOptions, "unix", cli);
        addUnixMacroStyle(result, languageOptions, "linux", cli);
        result.additionalMacros.emplace_back("__ELF__", "1");
        result.additionalMacros.emplace_back("__gnu_linux__", "1");
    }

    result.additionalMacros.emplace_back("__llvm__", "1");
    result.additionalMacros.emplace_back("__coral66__", "1");
    // TODO: Command line option for this
    result.additionalMacros.emplace_back("__GNUC__", "4");
    result.additionalMacros.emplace_back("__GNUC_MINOR__", "2");
    result.additionalMacros.emplace_back("__GNUC_PATCHLEVEL__", "1");
    if (cli.template get<OPT>() && *cli.template get<OPT>() > 0)
    {
        result.additionalMacros.emplace_back("__OPTIMIZE__", "1");
    }

    // TODO: Macro definitions that follow should be more generalized and not depend on hard coded known values of
    // targets

    result.additionalMacros.emplace_back("__ORDER_LITTLE_ENDIAN__", "1234");
    result.additionalMacros.emplace_back("__ORDER_BIG_ENDIAN__", "4321");

    result.additionalMacros.emplace_back("__BYTE_ORDER__", "__ORDER_LITTLE_ENDIAN__");
    result.additionalMacros.emplace_back("__LITTLE_ENDIAN__", "1");

    if (languageOptions.sizeOfInt == 4 && languageOptions.sizeOfLong == 8 && languageOptions.sizeOfVoidStar == 8)
    {
        result.additionalMacros.emplace_back("_LP64", "1");
        result.additionalMacros.emplace_back("__LP64__", "1");
    }
    else if (languageOptions.sizeOfVoidStar == 4 && languageOptions.sizeOfInt == 4 && languageOptions.sizeOfLong == 4)
    {
        result.additionalMacros.emplace_back("_ILP32", "1");
        result.additionalMacros.emplace_back("__ILP32__", "1");
    }

    result.additionalMacros.emplace_back("__CHAR_BIT__", "8");

    result.additionalMacros.emplace_back("__SCHAR_MAX__", "127");
    result.additionalMacros.emplace_back(
        "__SHRT_MAX__", coral66::to_string(safeShiftLeft(1uLL, (8 * languageOptions.sizeOfShort - 1)) - 1));
    result.additionalMacros.emplace_back("__INT_MAX__",
                                         coral66::to_string(safeShiftLeft(1uLL, (8 * languageOptions.sizeOfInt - 1)) - 1));
    result.additionalMacros.emplace_back("__LONG_MAX__",
                                         coral66::to_string(safeShiftLeft(1uLL, (8 * languageOptions.sizeOfLong - 1)) - 1));
    result.additionalMacros.emplace_back("__LONG_LONG_MAX__", coral66::to_string(safeShiftLeft(1ull, (8 * 8 - 1)) - 1));
    std::size_t const wcharSize = languageOptions.sizeOf(languageOptions.wcharUnderlyingType);
    bool const wcharSigned = coral66::LanguageOptions::isSigned(languageOptions.wcharUnderlyingType);
    result.additionalMacros.emplace_back("__WCHAR_MAX__",
                                         coral66::to_string(safeShiftLeft(1uLL, (8 * wcharSize - wcharSigned)) - 1));
    result.additionalMacros.emplace_back("__INTMAX_MAX__", "__LONG_LONG_MAX__");
    result.additionalMacros.emplace_back("__UINTMAX_MAX__", coral66::to_string(~0ull));

    std::size_t const sizeTSize = languageOptions.sizeOf(languageOptions.sizeTType);
    result.additionalMacros.emplace_back("__SIZE_MAX__", coral66::to_string(safeShiftLeft(1ull, (8 * sizeTSize)) - 1));
    std::size_t const ptrDiffSize = languageOptions.sizeOf(languageOptions.ptrdiffType);
    result.additionalMacros.emplace_back("__PTRDIFF_MAX___",
                                         coral66::to_string(safeShiftLeft(1ull, (8 * ptrDiffSize - 1)) - 1));
    result.additionalMacros.emplace_back(
        "__INTPTR_MAX__", coral66::to_string(safeShiftLeft(1ull, (8 * languageOptions.sizeOfVoidStar - 1)) - 1));
    result.additionalMacros.emplace_back("__UNTPTR_MAX__",
                                         coral66::to_string(safeShiftLeft(1ull, (8 * languageOptions.sizeOfVoidStar)) - 1));

    result.additionalMacros.emplace_back("__SIZEOF_DOUBLE__", "8");
    result.additionalMacros.emplace_back("__SIZEOF_FLOAT__", "4");
    result.additionalMacros.emplace_back("__SIZEOF_INT__", coral66::to_string(languageOptions.sizeOfInt));
    result.additionalMacros.emplace_back("__SIZEOF_LONG__", coral66::to_string(languageOptions.sizeOfLong));
    result.additionalMacros.emplace_back("__SIZEOF_LONG_DOUBLE__", coral66::to_string(16));
    result.additionalMacros.emplace_back("__SIZEOF_LONG_LONG__", "64");
    result.additionalMacros.emplace_back("__SIZEOF_POINTER__", coral66::to_string(languageOptions.sizeOfVoidStar));
    result.additionalMacros.emplace_back("__SIZEOF_SHORT__", coral66::to_string(languageOptions.sizeOfShort));
    result.additionalMacros.emplace_back("__SIZEOF_PTRDIFF_T__", coral66::to_string(ptrDiffSize));
    result.additionalMacros.emplace_back("__SIZEOF_SIZE_T__", coral66::to_string(sizeTSize));
    result.additionalMacros.emplace_back("__SIZEOF_WCHAR_T__", coral66::to_string(wcharSize));
    if (languageOptions.int128Enabled)
    {
        result.additionalMacros.emplace_back("__SIZEOF_INT128__", "128");
    }

    result.additionalMacros.emplace_back("__INTMAX_TYPE__", "long long");
    result.additionalMacros.emplace_back("__INTMAX_C_SUFFIX__", "LL");
    result.additionalMacros.emplace_back("__INTMAX_WIDTH____", "64");
    result.additionalMacros.emplace_back("__UINTMAX_TYPE__", "unsigned long long");
    result.additionalMacros.emplace_back("__UINTMAX_C_SUFFIX__", "ULL");
    result.additionalMacros.emplace_back("__UINTMAX_WIDTH__", "64");
    result.additionalMacros.emplace_back("__PTRDIFF_TYPE__",
                                         coral66::to_string(coral66::LanguageOptions::string(languageOptions.ptrdiffType)));
    result.additionalMacros.emplace_back("__PTRDIFF_WIDTH__", coral66::to_string(ptrDiffSize * 8));
    result.additionalMacros.emplace_back("__SIZE_TYPE__",
                                         coral66::to_string(coral66::LanguageOptions::string(languageOptions.sizeTType)));
    result.additionalMacros.emplace_back("__SIZE_WIDTH__", coral66::to_string(sizeTSize * 8));
    result.additionalMacros.emplace_back(
        "__WCHAR_TYPE__", coral66::to_string(coral66::LanguageOptions::string(languageOptions.wcharUnderlyingType)));
    result.additionalMacros.emplace_back("__WCHAR_WIDTH__", coral66::to_string(wcharSize * 8));

    result.additionalMacros.emplace_back("__POINTER__WIDTH__", coral66::to_string(8 * languageOptions.sizeOfVoidStar));
    if (!languageOptions.charIsSigned)
    {
        result.additionalMacros.emplace_back("__CHAR_UNSIGNED__", "1");
    }

    if (!coral66::LanguageOptions::isSigned(languageOptions.wcharUnderlyingType))
    {
        result.additionalMacros.emplace_back("__WCHAR_UNSIGNED__", "1");
    }

    result.additionalMacros.emplace_back("__FINITE_MATH_ONLY__", "0");
    result.additionalMacros.emplace_back("__GNUC_STDC_INLINE__", "1");
    if (languageOptions.extension == coral66::LanguageOptions::Extension::None)
    {
        result.additionalMacros.emplace_back("__STRICT_ANSI__", "1");
    }
    return result;
}

template <auto& Value>
struct ValueIdentity
{
    constexpr static auto& value = Value;
};

template <class CL>
void setTargetFeatures(coral66::TargetFeatures& targetFeatures, const CL& cli, llvm::raw_ostream* reporter) // NOLINT(*-prefer-static-over-anonymous-namespace)
{
    auto tryApply = [&]<typename T0>([[maybe_unused]]T0 opt, coral66::TargetFeatures::Features const feature)
    {
        constexpr auto& Opt = std::decay_t<T0>::value;
        if (cli.template pos<Opt>() >= 0)
        {
            if (!targetFeatures.setFeature(feature, cli.template get<Opt>()))
            {
                if (reporter)
                {
                    *reporter << coral66::Warnings::CLI::FLAG_N_IS_NOT_APPLICABLE_FOR_CURRENT_TARGET_ARCHITECTURE.argsCLI(
                        Opt.getFirstOptionString());
                }
            }
            else
            {
                return cli.template get<Opt>();
            }
        }
        return false;
    };

    tryApply(ValueIdentity<MMX>{}, coral66::TargetFeatures::MMX);
    tryApply(ValueIdentity<SSE>{}, coral66::TargetFeatures::SSE);
    if (tryApply(ValueIdentity<THREEDNOWA>{}, coral66::TargetFeatures::ThreeDNowA))
    {
        targetFeatures.setFeature(coral66::TargetFeatures::ThreeDNow, true);
        targetFeatures.setFeature(coral66::TargetFeatures::MMX, true);
    }
    if (tryApply(ValueIdentity<SSE2>{}, coral66::TargetFeatures::SSE2))
    {
        targetFeatures.setFeature(coral66::TargetFeatures::SSE, true);
    }
    if (tryApply(ValueIdentity<SSE3>{}, coral66::TargetFeatures::SSE3)
        || tryApply(ValueIdentity<AES>{}, coral66::TargetFeatures::AES)
        || tryApply(ValueIdentity<AES>{}, coral66::TargetFeatures::PCLMUL))
    {
        targetFeatures.setFeature(coral66::TargetFeatures::SSE, true);
        targetFeatures.setFeature(coral66::TargetFeatures::SSE2, true);
    }
    if (tryApply(ValueIdentity<SSSE3>{}, coral66::TargetFeatures::SSSE3)
        || tryApply(ValueIdentity<SSSE3>{}, coral66::TargetFeatures::SSE4a))
    {
        targetFeatures.setFeature(coral66::TargetFeatures::SSE, true);
        targetFeatures.setFeature(coral66::TargetFeatures::SSE2, true);
        targetFeatures.setFeature(coral66::TargetFeatures::SSE3, true);
    }
    if (tryApply(ValueIdentity<SSE4_1>{}, coral66::TargetFeatures::SSE4_1))
    {
        targetFeatures.setFeature(coral66::TargetFeatures::SSE, true);
        targetFeatures.setFeature(coral66::TargetFeatures::SSE2, true);
        targetFeatures.setFeature(coral66::TargetFeatures::SSE3, true);
        targetFeatures.setFeature(coral66::TargetFeatures::SSSE3, true);
    }
    if (tryApply(ValueIdentity<SSE4_2>{}, coral66::TargetFeatures::SSE4_2))
    {
        targetFeatures.setFeature(coral66::TargetFeatures::SSE, true);
        targetFeatures.setFeature(coral66::TargetFeatures::SSE2, true);
        targetFeatures.setFeature(coral66::TargetFeatures::SSE3, true);
        targetFeatures.setFeature(coral66::TargetFeatures::SSSE3, true);
        targetFeatures.setFeature(coral66::TargetFeatures::SSE4_1, true);
    }
    if (tryApply(ValueIdentity<AVX>{}, coral66::TargetFeatures::AVX))
    {
        targetFeatures.setFeature(coral66::TargetFeatures::SSE, true);
        targetFeatures.setFeature(coral66::TargetFeatures::SSE2, true);
        targetFeatures.setFeature(coral66::TargetFeatures::SSE3, true);
        targetFeatures.setFeature(coral66::TargetFeatures::SSSE3, true);
        targetFeatures.setFeature(coral66::TargetFeatures::SSE4_1, true);
        targetFeatures.setFeature(coral66::TargetFeatures::SSE4_2, true);
    }
    if (tryApply(ValueIdentity<AVX>{}, coral66::TargetFeatures::AVX2))
    {
        targetFeatures.setFeature(coral66::TargetFeatures::SSE, true);
        targetFeatures.setFeature(coral66::TargetFeatures::SSE2, true);
        targetFeatures.setFeature(coral66::TargetFeatures::SSE3, true);
        targetFeatures.setFeature(coral66::TargetFeatures::SSSE3, true);
        targetFeatures.setFeature(coral66::TargetFeatures::SSE4_1, true);
        targetFeatures.setFeature(coral66::TargetFeatures::SSE4_2, true);
        targetFeatures.setFeature(coral66::TargetFeatures::AVX, true);
    }
    if (tryApply(ValueIdentity<FMA4>{}, coral66::TargetFeatures::Fma4))
    {
        targetFeatures.setFeature(coral66::TargetFeatures::SSE, true);
        targetFeatures.setFeature(coral66::TargetFeatures::SSE2, true);
        targetFeatures.setFeature(coral66::TargetFeatures::SSE3, true);
        targetFeatures.setFeature(coral66::TargetFeatures::SSSE3, true);
        targetFeatures.setFeature(coral66::TargetFeatures::SSE4_1, true);
        targetFeatures.setFeature(coral66::TargetFeatures::SSE4_2, true);
        targetFeatures.setFeature(coral66::TargetFeatures::SSE4a, true);
        targetFeatures.setFeature(coral66::TargetFeatures::AVX, true);
    }
    if (tryApply(ValueIdentity<XOPA>{}, coral66::TargetFeatures::XOP))
    {
        targetFeatures.setFeature(coral66::TargetFeatures::SSE, true);
        targetFeatures.setFeature(coral66::TargetFeatures::SSE2, true);
        targetFeatures.setFeature(coral66::TargetFeatures::SSE3, true);
        targetFeatures.setFeature(coral66::TargetFeatures::SSSE3, true);
        targetFeatures.setFeature(coral66::TargetFeatures::SSE4_1, true);
        targetFeatures.setFeature(coral66::TargetFeatures::SSE4_2, true);
        targetFeatures.setFeature(coral66::TargetFeatures::SSE4a, true);
        targetFeatures.setFeature(coral66::TargetFeatures::AVX, true);
        targetFeatures.setFeature(coral66::TargetFeatures::Fma4, true);
    }
    if (tryApply(ValueIdentity<XSAVEOPT>{}, coral66::TargetFeatures::XSaveOpt))
    {
        targetFeatures.setFeature(coral66::TargetFeatures::XSave, true);
    }
    if (tryApply(ValueIdentity<THREEDNOW>{}, coral66::TargetFeatures::ThreeDNow))
    {
        targetFeatures.setFeature(coral66::TargetFeatures::MMX, true);
    }
    tryApply(ValueIdentity<FSGSBASE>{}, coral66::TargetFeatures::FSGSBase);
    tryApply(ValueIdentity<RDRND>{}, coral66::TargetFeatures::RDrnd);
    tryApply(ValueIdentity<PTWRITE>{}, coral66::TargetFeatures::PTWrite);
    tryApply(ValueIdentity<LWP>{}, coral66::TargetFeatures::LWP);
    tryApply(ValueIdentity<BMI>{}, coral66::TargetFeatures::BMI);
    tryApply(ValueIdentity<BMI2>{}, coral66::TargetFeatures::BMI2);
    tryApply(ValueIdentity<LZCNT>{}, coral66::TargetFeatures::LZCnt);
    tryApply(ValueIdentity<FXSR>{}, coral66::TargetFeatures::FXSr);
    tryApply(ValueIdentity<XSAVE>{}, coral66::TargetFeatures::XSave);
    tryApply(ValueIdentity<TBM>{}, coral66::TargetFeatures::TBM);
    tryApply(ValueIdentity<RTM>{}, coral66::TargetFeatures::RTM);
    tryApply(ValueIdentity<MWAITX>{}, coral66::TargetFeatures::MWaitX);
    tryApply(ValueIdentity<CLZERO>{}, coral66::TargetFeatures::CLZero);
    tryApply(ValueIdentity<PKU>{}, coral66::TargetFeatures::PKU);
}

enum class Action
{
    Compile,
    Preprocess,
    AssemblyOutput,
    Link [[maybe_unused]]
};

template <class CL>
std::optional<coral66::fs::path> compileCFile(Action action, coral66::fs::path cSourceFile, const coral66::Triple& triple, // NOLINT(*-prefer-static-over-anonymous-namespace)
                                          const coral66::LanguageOptions& languageOptions, const CL& cli,
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

    std::size_t const size = file.tellg();
    file.seekg(0);
    std::string input(size, '\0');
    file.read(input.data(), size);
    file.close();

    cSourceFile = coral66::fs::absolute(cSourceFile);
    cSourceFile = cSourceFile.lexically_normal();

    std::optional<llvm::TimeRegion> ppTokenTimeRegion;
    if (timer)
    {std::optional<llvm::Timer> ppTokenTimer;
        ppTokenTimer.emplace("Lexing preprocessor tokens",
                             "Time it took to lex all preprocessor tokens in " + cSourceFile.string(), *timer);
        ppTokenTimeRegion.emplace(*ppTokenTimer);
    }

    bool errors = false;
    auto pptokens = coral66::Lexer::tokenize(std::move(input), &languageOptions, reporter, &errors, cSourceFile.string());
    if (errors)
    {
        return {};
    }
    ppTokenTimeRegion.reset();

    std::optional<llvm::TimeRegion> ppTimeRegion;
    if (timer)
    {std::optional<llvm::Timer> ppTimer;
        ppTimer.emplace("Preprocessor", "Time it took to preprocess " + cSourceFile.string(), *timer);
        ppTimeRegion.emplace(*ppTimer);
    }

    coral66::PP::Options ppOptions = getTargetSpecificPreprocessorOptions(languageOptions, triple, cli);
    for (auto& iter : cli.template get<INCLUDES>())
    {
        if (std::none_of(cli.template get<ISYSTEM>().begin(), cli.template get<ISYSTEM>().end(),
                         [&iter](const coral66::fs::path& path)
                         {
                             std::error_code ec;
                             auto res = coral66::fs::equivalent(path, coral66::fs::path(iter), ec);
                             return res && !ec;
                         }))
        {
            ppOptions.includeDirectories.emplace_back(iter);
        }
    }
    ppOptions.systemDirectories = cli.template get<ISYSTEM>();
    for (auto& [name, maybeValue] : cli.template get<DEFINE_MACRO>())
    {
        ppOptions.additionalMacros.emplace_back(name, maybeValue.value_or("1"));
    }
    pptokens = coral66::PP::preprocess(std::move(pptokens), ppOptions, reporter, &errors);
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
    {std::optional<llvm::Timer> cTokenTimer;
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
    {std::optional<llvm::Timer> parserTimer;
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
    {std::optional<llvm::Timer> semanticsTimer;
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
    {std::optional<llvm::Timer> codegenTimer;
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
    codegenOptions.emitAllDecls = cli.template get<EMIT_ALL_DECLS>();
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
    {std::optional<llvm::Timer> compileTimer;
        compileTimer.emplace("Compile",
                             "Time it took for LLVM to generate native object code " + cSourceFile.string(), *timer);
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
                                               action == Action::AssemblyOutput ?
                                                   llvm::CodeGenFileType::AssemblyFile :
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
int doActionOnAllFiles(Action action, const coral66::LanguageOptions& languageOptions, const coral66::Triple& triple, // NOLINT(*-prefer-static-over-anonymous-namespace)
                       std::span<const std::string_view> files, const CL& cli, llvm::raw_ostream* reporter)
{
    std::vector<coral66::fs::path> linkableFiles;
    for (auto& iter : files)
    {
        auto path = coral66::fs::path(iter);
        auto extension = path.extension();
        if (extension == ".c")
        {
            auto objectFile = compileCFile(action, iter, triple, languageOptions, cli, reporter);
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
    os << "coral66 version " << CLD_VERSION << '\n';
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

    auto cli = coral66::parseCommandLine<OUTPUT_FILE, COMPILE_ONLY, ASSEMBLY_OUTPUT, PREPROCESS_ONLY, TARGET, EMIT_LLVM,
                                     OPT, DEFINE_MACRO, INCLUDES, PIE, PIC, FREESTANDING, HELP, VERSION,
                                     STANDARD_VERSION, WARNINGS, ISYSTEM, EMIT_ALL_DECLS, G0, G1, G2, G3, SYNTAX_ONLY,
                                     TIME, MMX, SSE, THREEDNOWA, SSE2, SSE3, SSSE3, SSE4_1, SSE4_2, AVX, AVX2, AES,
                                     PCLMUL, FSGSBASE, RDRND, PTWRITE, SSE4A, XOPA, FMA4, LWP, BMI, BMI2, LZCNT, FXSR,
                                     XSAVE, XSAVEOPT, TBM, THREEDNOW, RTM, MWAITX, CLZERO, PKU>(elements);
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
    auto triple = coral66::Triple::defaultTarget();
    if (cli.get<TARGET>())
    {
        triple = coral66::Triple::fromString(*cli.get<TARGET>());
    }
    bool gnuExtensions = false;
    auto language = coral66::LanguageOptions::Language::C99;
    if (cli.get<STANDARD_VERSION>())
    {
        auto str = *cli.get<STANDARD_VERSION>();
        if (str == "gnu99")
        {
            language = coral66::LanguageOptions::Language ::C99;
            gnuExtensions = true;
        }
        else if (str == "c99")
        {
            language = coral66::LanguageOptions::Language ::C99;
            gnuExtensions = false;
        }
        else
        {
            if (reporter)
            {
                *reporter << Errors::CLI::UNKNOWN_LANGUAGE_STANDARD_N.argsCLI(str);
            }
            return -1;
        }
    }
    auto options = coral66::LanguageOptions::fromTriple(triple, language);
    if (gnuExtensions)
    {
        options.extension = coral66::LanguageOptions::Extension::GNU;
    }
    options.enabledWarnings = cli.get<WARNINGS>();
    options.freeStanding = cli.get<FREESTANDING>();
    setTargetFeatures(*options.targetFeatures, cli, reporter);

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
        return doActionOnAllFiles(Action::Compile, options, triple, cli.getUnrecognized(), cli, reporter);
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
        return doActionOnAllFiles(Action::AssemblyOutput, options, triple, cli.getUnrecognized(), cli, reporter);
    }
    if (cli.get<PREPROCESS_ONLY>())
    {
        return doActionOnAllFiles(Action::Preprocess, options, triple, cli.getUnrecognized(), cli, reporter);
    }
    return doActionOnAllFiles(Action::Compile, options, triple, cli.getUnrecognized(), cli, reporter);
}
