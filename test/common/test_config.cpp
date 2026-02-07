/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "test/common/test_config.hpp"

#include <llvm/Support/Process.h>

coral66::Tests::FileScope::~FileScope()
{
    coral66::fs::remove(m_path);
}

coral66::Tests::FileScope coral66::Tests::createInclude(std::string_view path, std::string_view content)
{
    auto dir = coral66::fs::path(path.data());
    dir.remove_filename();
    if (!dir.empty())
    {
        coral66::fs::create_directories(dir);
    }
    auto utf8path = coral66::fs::path(path);
    coral66::fs::ofstream file(utf8path, std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
    REQUIRE(file.is_open());
    file.write(content.data(), content.size());
    return FileScope(utf8path);
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
