#define CATCH_CONFIG_MAIN
#include "vendor/catch2v3/catch_amalgamated.hpp"
#include <sstream>
#include <iostream>

// Custom stream wrapper to capture output
class StringStreamWrapper : public Catch::IStream {
    std::stringstream& m_ss;
public:
    StringStreamWrapper(std::stringstream& ss) : m_ss(ss) {}
    std::ostream& stream() override { return m_ss; }
};

TEST_CASE("Examine Console Reporter Output", "[reporter]") {
    std::stringstream ss;

    auto config = Catch::Detail::make_unique<Catch::Config>();

    Catch::ReporterConfig reporterConfig(
        config.get(),
        Catch::Detail::make_unique<StringStreamWrapper>(ss),
        Catch::ColourMode::None,
        {} 
    );

    Catch::ConsoleReporter reporter(std::move(reporterConfig));

    Catch::TestRunInfo runInfo("MyCustomTestRun");
    reporter.testRunStarting(runInfo);

    Catch::SourceLineInfo lineInfo("test_file.cpp", 123);
    Catch::NameAndTags nameAndTags("MyTestCase", "[tag]");
    Catch::TestCaseInfo testCaseInfo("MyClass", nameAndTags, lineInfo);
    
    reporter.testCaseStarting(testCaseInfo);

    // KEY STEP: Push the root section for the test case
    Catch::SectionInfo sectionInfo(lineInfo, "MyTestCase");
    reporter.sectionStarting(sectionInfo);

    Catch::AssertionResultData resultData(Catch::ResultWas::ExpressionFailed, Catch::LazyExpression(false));
    resultData.message = "This assertion failed intentionally";
    
    Catch::AssertionInfo info = { "CHECK", lineInfo, "false", Catch::ResultDisposition::ContinueOnFailure };
    
    Catch::AssertionResult assertionResult(info, std::move(resultData));
    
    Catch::Totals totals;
    totals.assertions.failed = 1;
    
    Catch::AssertionStats assertionStats(assertionResult, {}, totals);
    reporter.assertionEnded(assertionStats);
    
    totals.testCases.failed = 1;

    // End the root section
    Catch::SectionStats sectionStats(std::move(sectionInfo), totals.assertions, 0.0, false);
    reporter.sectionEnded(sectionStats);
    
    Catch::TestCaseStats testCaseStats(
        testCaseInfo,
        totals,
        "", 
        "", 
        false 
    );
    reporter.testCaseEnded(testCaseStats);

    Catch::TestRunStats runStats(runInfo, totals, false);
    reporter.testRunEnded(runStats);

    std::string output = ss.str();
    
    REQUIRE(output.find("MyTestCase") != std::string::npos);
    REQUIRE(output.find("This assertion failed intentionally") != std::string::npos);
    REQUIRE(output.find("test_file.cpp") != std::string::npos);
}
