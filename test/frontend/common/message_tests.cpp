/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 11/08/2026
 */

#include "lib/frontend/common/messages/message.hpp"

#include <llvm/Support/raw_ostream.h>

#include "test/vendor/catch2v3/catch_amalgamated.hpp"

TEST_CASE("Message construction and segment validation", "[Message]")
{
    SECTION("Empty message has no segments")
    {
        coral66::Message msg;
        CHECK(msg.getSeverity() == coral66::Severity::None);
        CHECK(msg.getText().empty());
        CHECK(msg.getSegments().empty());
    }

    SECTION("String constructor builds single text without segments")
    {
        coral66::Message msg(coral66::Severity::Error, "Test error");
        CHECK(msg.getSeverity() == coral66::Severity::Error);
        CHECK(msg.getText() == "Test error");
        CHECK(msg.getSegments().empty());
    }

    SECTION("Segment constructor builds text correctly")
    {
        std::vector<coral66::Message::Segment> segments = {
            {.text = "Error: ", .color = llvm::raw_ostream::RED, .bold = true, .bg = false},
            {.text = "Something went wrong", .color = llvm::raw_ostream::SAVEDCOLOR, .bold = false, .bg = false}};
        coral66::Message msg(coral66::Severity::Warning, std::move(segments));

        CHECK(msg.getSeverity() == coral66::Severity::Warning);
        CHECK(msg.getText() == "Error: Something went wrong");
        REQUIRE(msg.getSegments().size() == 2);
        CHECK(msg.getSegments()[0].text == "Error: ");
        CHECK(msg.getSegments()[0].color == llvm::raw_ostream::RED);
        CHECK(msg.getSegments()[0].bold == true);

        CHECK(msg.getSegments()[1].text == "Something went wrong");
        CHECK(msg.getSegments()[1].color == llvm::raw_ostream::SAVEDCOLOR);
    }
}

TEST_CASE("Message segment streaming with colors_enabled", "[Message]")
{
    std::vector<coral66::Message::Segment> segments = {
        {.text = "RedBold", .color = llvm::raw_ostream::RED, .bold = true, .bg = false},
        {.text = "Normal", .color = llvm::raw_ostream::SAVEDCOLOR, .bold = false, .bg = false}};
    coral66::Message msg(coral66::Severity::Error, std::move(segments));

    SECTION("Streaming with colors disabled")
    {
        std::string out;
        llvm::raw_string_ostream os(out);
        os.enable_colors(false);
        os << msg;
        os.flush();

        // When colors are disabled, no ANSI codes should be emitted.
        CHECK(out == "RedBoldNormal");
    }

    SECTION("Streaming with colors enabled")
    {
        std::string out;
        llvm::raw_string_ostream os(out);
        os.enable_colors(true);
        os << msg;
        os.flush();

        // When colors are enabled, we expect ANSI codes around RedBold.
        // llvm::raw_ostream outputs specific codes. We just verify they are present.
        CHECK(out.find("RedBold") != std::string::npos);
        CHECK(out.find("Normal") != std::string::npos);
        CHECK(out.find("\033[") != std::string::npos); // Verify ANSI escape was used
        CHECK(out != "RedBoldNormal");                 // Verify it is different from plain text
    }
}
