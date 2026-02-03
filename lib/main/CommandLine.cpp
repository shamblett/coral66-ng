/*
* Project: coral66
* Author : Copyright S. Hamblett <steve.hamblett@linux.com>
* Date   : 31/01/2026
*/

#include "CommandLine.hpp"

#include <llvm/Support/raw_ostream.h>

#include "lib/frontend/common/ErrorMessages.hpp"

void coral66::detail::CommandLine::printHelp(llvm::raw_ostream& os,
                                         std::vector<std::pair<std::string_view, std::string_view>> options)
{
    if (options.empty())
    {
        return;
    }
    const auto maxLen =
        std::ranges::max_element(options.begin(), options.end(),
                         [](const auto& lhs, const auto& rhs) { return lhs.first.size() < rhs.first.size(); })
            ->first.size();
    constexpr auto indentStep = 4;
    auto width = roundUpTo(maxLen, indentStep);

    for (auto [cli, description] : options)
    {
        os.write(cli.data(), cli.size());
        os.indent(width - cli.size() + indentStep).write(description.data(), description.size()) << '\n';
    }
    os.flush();
}

namespace
{
std::string reconstructCommand(std::size_t const currentIndex, std::size_t const currentPos, // NOLINT(*-prefer-static-over-anonymous-namespace)
                               std::span<const std::string_view> const commandLine)
{
    std::string command;
    for (std::size_t i = 0; i <= currentIndex; i++)
    {
        if (i != 0)
        {
            command += " ";
        }
        if (i != currentIndex)
        {
            command += commandLine[i];
        }
        else
        {
            command += commandLine[i].substr(0, currentPos);
        }
    }
    return command;
}
} // namespace

coral66::Message coral66::detail::CommandLine::emitConsumeFailure(std::size_t const currentIndex, std::size_t const currentPos,
                                                          std::span<const std::string_view> const commandLine,
                                                          std::string_view text)
{
    auto command = reconstructCommand(currentIndex, currentPos, commandLine);
    return Errors::CLI::EXPECTED_N_AFTER_N.argsCLI(text, command);
}

coral66::Message coral66::detail::CommandLine::emitMissingArg(std::size_t const currentIndex, std::size_t const currentPos,
                                                      std::span<const std::string_view> const commandLine,
                                                      bool immediatelyAfter)
{
    auto command = reconstructCommand(currentIndex, currentPos, commandLine);
    if (immediatelyAfter)
    {
        return Errors::CLI::EXPECTED_ARGUMENT_IMMEDIATELY_AFTER_N.argsCLI(command);
    }

    return Errors::CLI::EXPECTED_ARGUMENT_AFTER_N.argsCLI(command);
}

coral66::Message coral66::detail::CommandLine::emitMissingWhitespace(std::size_t const currentIndex, std::size_t const currentPos,
                                                             std::span<const std::string_view> const commandLine)
{
    auto command = reconstructCommand(currentIndex, currentPos, commandLine);
    return Errors::CLI::EXPECTED_WHITESPACE_AFTER_N.argsCLI(command);
}

coral66::Message coral66::detail::CommandLine::emitFailedInteger(std::size_t const currentIndex, std::size_t const currentPos,
                                                         std::span<const std::string_view> const commandLine)
{
    auto command = reconstructCommand(currentIndex, currentPos, commandLine);
    return Errors::CLI::ERRORS_PARSING_INTEGER_ARGUMENT_IN_N.argsCLI(command);
}

coral66::Message coral66::detail::CommandLine::emitInvalidUTF8(std::size_t const currentIndex, std::size_t const currentPos,
                                                       std::span<const std::string_view> const commandLine)
{
    auto command = reconstructCommand(currentIndex, currentPos, commandLine);
    return Errors::CLI::ERRORS_PARSING_INVALID_UTF8_IN_N.argsCLI(command);
}
