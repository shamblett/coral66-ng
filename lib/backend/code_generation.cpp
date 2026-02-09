/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "code_generation.hpp"

#include <llvm/MC/TargetRegistry.h>

#include "code_generator.hpp"

namespace coral66::CGLLVM
{

std::unique_ptr<llvm::TargetMachine> generateLLVM(llvm::Module& module, const Semantics::Program& program,
                                                  const Options& options)
{
    // Minimal SourceInterface stub or pass nullptr if not needed by basic codegen yet
    // The previous code_generator constructor took SourceInterface.
    // We can pass a dummy one or update Codegen.hpp to take SourceInterface?
    // code_generator.hpp needs SourceInterface.
    // For now, let's assume caller provides it, or we stub it.
    // But generateLLVM signature in Codegen.hpp does NOT take SourceInterface.
    // This is an issue. I should update Codegen.hpp or remove dependency.

    // Stub SourceInterface
    class StubSourceInterface : public source_interface
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

    code_generator generator(module, program, stubSource, options);
    generator.generate();

    // Return target machine (stub)
    std::string error;
    auto target = llvm::TargetRegistry::lookupTarget(module.getTargetTriple(), error);
    if (!target)
        return nullptr;
    return std::unique_ptr<llvm::TargetMachine>(
        target->createTargetMachine(module.getTargetTriple(), "generic", "", {}, {}));
}

} // namespace coral66::CGLLVM
