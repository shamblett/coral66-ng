/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 16/05/2026
 */

#include "lib/frontend/semantics/program.hpp"

#include <iostream>

#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include "lib/frontend/common/source_object.hpp"
#include "lib/frontend/parser/parser.hpp"
#include "lib/frontend/preprocessor/preprocessor.hpp"
#include "lib/support/filesystem.hpp"
#include "sysexits.h"
#include "tools/semantics/programdump/lib/program_dump.hpp"

static std::string readInputFile(const std::string& inputFile)
{
    coral66::fs::ifstream file(inputFile, std::ios_base::in | std::ios_base::ate);
    if (!file.is_open())
    {
        std::cout << "SemanticsProgramDump::Could not open input source file " << inputFile << std::endl;
        return "";
    }

    std::streamsize const size = file.tellg();
    file.seekg(0);
    std::string input(size, '\0');
    file.read(input.data(), size);
    file.close();

    return input;
}

static void generateOutputFile(const std::string& inputFile, const rapidjson::StringBuffer& buffer)
{
    coral66::fs::path input_path = inputFile;
    coral66::fs::path output_path = input_path.parent_path() / (input_path.stem().string() + "_program");
    output_path.replace_extension(".json");

    coral66::fs::ofstream outputFile(output_path, std::ios_base::out);
    if (!outputFile.is_open())
    {
        std::cout << "SemanticsProgramDump::Could not open output file " << output_path << std::endl;
    }
    outputFile << buffer.GetString();
    outputFile.close();
}

int main(const int argc, char** argv)
{
    // Check the arguments
    if (argc != 2)
    {
        std::cout << "SemanticsProgramDump::Usage: " << argv[0] << "<input_file>" << std::endl;
        return EX_USAGE;
    }
    // Read the input source file
    std::string inputFile(argv[1]);
    auto source = readInputFile(std::string(inputFile));
    if (source.empty())
    {
        std::cout << "SemanticsProgramDump::Could not read input source file " << argv[1] << std::endl;
        return EX_NOINPUT;
    }

    // Parse the input
    std::string string;
    llvm::raw_string_ostream ss(string);
    coral66::PPSourceObject tokens;
    bool errors = false;

    // Let diagnostics percolate downwards, we output them in the program dump.
    // Lex
    tokens = coral66::lexer::tokenize(source, &ss, &errors);

    // Preprocess
    auto result = coral66::pp::preprocess(std::move(tokens), &ss, &errors);

    // Parse
    auto cTokens = coral66::lexer::toCTokens(std::move(result), &ss, &errors);

    auto tree = coral66::parser::parseProgram(cTokens, &ss, &errors);

    // Analyze
    auto program = coral66::semantics::analyse(tree, std::move(cTokens), &ss, &errors);

    // Create the RapidJSON Document
    rapidjson::Document doc;
    doc.SetObject();

    coral66::tools::ProgramDump dump(doc.GetAllocator());

    // Dump the AST into the RapidJSON document root
    rapidjson::Value root = dump.dump(program, ss.str());
    doc.Swap(root);

    // Serialize to string for printing
    rapidjson::StringBuffer buffer;

    // Use PrettyWriter instead of Writer for human-readable output
    rapidjson::PrettyWriter writer(buffer);
    doc.Accept(writer);

    // Generate the output file
    generateOutputFile(inputFile, buffer);

    return EX_OK;
}
