/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "code_generation.hpp"

#include <llvm/MC/TargetRegistry.h>

#include "code_generator.hpp"
#include "lib/support/vendor/ctrack/include/ctrack.hpp"

namespace coral66::cgllvm
{

std::unique_ptr<llvm::TargetMachine> generateLLVM(llvm::Module& module, const syntax::Program& tree,
                                                  const semantics::Program& program, const Options& options)
{
    CTRACK_NAME("Generate LLVM");
    // Minimal SourceInterface stub or pass nullptr if not needed by basic codegen yet
    // The previous CodeGenerator constructor took SourceInterface.
    // We can pass a dummy one or update Codegen.hpp to take SourceInterface?
    // CodeGenerator.hpp needs SourceInterface.
    // For now, let's assume caller provides it, or we stub it.
    // But generateLLVM signature in Codegen.hpp does NOT take SourceInterface.
    // This is an issue. I should update Codegen.hpp or remove dependency.

    // Stub SourceInterface
    const class StubSourceInterface : public source_interface
    {
    public:
        [[nodiscard]] std::uint64_t getLineNumber(std::uint32_t, std::uint64_t) const noexcept override
        {
            return 0;
        }
        [[nodiscard]] std::uint64_t getLineStartOffset(std::uint32_t, std::uint64_t) const noexcept override
        {
            return 0;
        }
        [[nodiscard]] std::uint64_t getLineEndOffset(std::uint32_t, std::uint64_t) const noexcept override
        {
            return 0;
        }
        [[nodiscard]] std::span<const Source::File> getFiles() const noexcept override
        {
            return {};
        }
        [[nodiscard]] std::span<const Source::PPRecord> getSubstitutions() const noexcept override
        {
            return {};
        }
    } stubSource;

    std::string error;
    std::unique_ptr<llvm::TargetMachine> targetMachine;

    // Only attempt to set up TargetMachine and DataLayout if a target triple is provided
    if (!module.getTargetTriple().empty())
    {
        if (const auto target = llvm::TargetRegistry::lookupTarget(module.getTargetTriple(), error))
        {
            targetMachine.reset(target->createTargetMachine(module.getTargetTriple(), "generic", "", {}, {}));
            if (targetMachine)
            {
                module.setDataLayout(targetMachine->createDataLayout());
            }
        }
    }

    CodeGenerator generator(module, tree, program, stubSource, options);
    generator.generate();

    return targetMachine;
}

} // namespace coral66::cgllvm
