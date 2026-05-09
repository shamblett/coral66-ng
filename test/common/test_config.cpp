/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "test/common/test_config.hpp"

#include <llvm/Support/Process.h>

#include <tuple>

#include <wordexp.h>

FileScope::~FileScope()
{
    fs::remove(m_path);
}

EndTest coral66::Tests::enableParallelization()
{
    const auto id = llvm::sys::Process::getProcessId();
    const auto path = fs::current_path() / std::to_string(id);
    coral66::fs::create_directory(path);
    coral66::fs::current_path(path);
    return EndTest{path};
}

EndTest::~EndTest()
{
    coral66::fs::current_path(path / "..");
    coral66::fs::remove_all(path);
}

std::tuple<int, char**> coral66::Tests::createArgs(const std::string& commandLine)
{
    const std::string cl = "coral66 " + commandLine;
    wordexp_t p;
    wordexp(cl.c_str(), &p, 0);
    return std::make_tuple(p.we_wordc, p.we_wordv);
}

std::string coral66::Tests::getTestDataPath(const std::string& relativePath, const std::string& testPathEnvVar)
{
    if (const auto srcDir = llvm::sys::Process::GetEnv(testPathEnvVar))
    {
        fs::path path = *srcDir;
        if (path.is_relative())
        {
            if (const auto testSrcDir = llvm::sys::Process::GetEnv("TEST_SRCDIR"))
            {
                if (const auto testWorkspace = llvm::sys::Process::GetEnv("TEST_WORKSPACE"))
                {
                    fs::path absolutePath = *testSrcDir;
                    absolutePath /= *testWorkspace;
                    absolutePath /= path;
                    path = absolutePath;
                }
            }
        }
        path /= relativePath;
        return path.string();
    }
    return relativePath;
}

std::string coral66::Tests::readFile(const std::string& path)
{
    fs::ifstream file(path, std::ios_base::in | std::ios_base::ate);
    if (!file.is_open())
    {
        return "";
    }

    std::streamsize const size = file.tellg();
    file.seekg(0);
    std::string input(size, '\0');
    file.read(input.data(), size);
    file.close();

    return input;
}
