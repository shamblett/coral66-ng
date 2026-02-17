/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 17/02/2026
 */

#pragma once

#include <llvm/Support/raw_ostream.h>

#include "lib/support/filesystem.hpp"

/**
 * @brief Main processing action executor functionality.
 *
 * Executes the actions as selected by the user, e.g. compile.
 */

namespace coral66
{
enum class Action
{
    Compile,
    Preprocess,
    AssemblyOutput,
    Link [[maybe_unused]]
};

class ActionExecutor
{
    llvm::raw_ostream* m_reporter;
    std::vector<fs::path> m_linkableFiles;

public:

    explicit ActionExecutor(llvm::raw_ostream* reporter) : m_reporter(reporter) {}

    ~ActionExecutor() = default;
    ActionExecutor(const ActionExecutor& other) = delete;
    ActionExecutor(ActionExecutor&& other) noexcept = delete;

    int execute(Action action, const std::string& file);

    std::optional<std::string> compileCoral66File(Action action, fs::path& sourceFile) const;
};
} // namespace coral66