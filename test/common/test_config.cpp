/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "test/common/test_config.hpp"

#include <tuple>
#include <wordexp.h>

#include <llvm/Support/Process.h>

coral66::Tests::FileScope::~FileScope()
{
    coral66::fs::remove(m_path);
}

EndTest coral66::Tests::enableParallelization()
{
    auto id = llvm::sys::Process::getProcessId();
    auto path = coral66::fs::current_path() / std::to_string(id);
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
    std::string cl = "coral66 " + commandLine;
    wordexp_t p;
    wordexp(cl.c_str(), &p, 0);
    return std::make_tuple(p.we_wordc, p.we_wordv);
}