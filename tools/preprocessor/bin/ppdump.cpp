/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 14/04/2026
 */

#include <iostream>

#include "lib/frontend/common/source_object.hpp"
#include "lib/frontend/parser/parser.hpp"
#include "lib/frontend/preprocessor/preprocessor.hpp"
#include "lib/support/filesystem.hpp"
#include "sysexits.h"

std::string readInputFile(const std::string& inputFile)
{
    coral66::fs::ifstream file(inputFile, std::ios_base::in | std::ios_base::ate);
    if (!file.is_open())
    {
        std::cout << "PPDump::Could not open input source file " << inputFile << std::endl;
        return "";
    }

    std::streamsize const size = file.tellg();
    file.seekg(0);
    std::string input(size, '\0');
    file.read(input.data(), size);
    file.close();

    return input;
}

void generateOutputFile(const std::string& inputFile, const std::string& buffer)
{
    coral66::fs::path input_path = inputFile;
    coral66::fs::path output_path = input_path.parent_path() / (input_path.stem().string() + "_pp");
    output_path.replace_extension(".txt");

    coral66::fs::ofstream outputFile(output_path, std::ios_base::out);
    if (!outputFile.is_open())
    {
        std::cout << "PPDump::Could not open output file " << output_path << std::endl;
    }
    outputFile << buffer;
    outputFile.close();
}

int main(const int argc, char** argv)
{
    // Check the arguments
    if (argc != 2)
    {
        std::cout << "PPDump::Usage: " << argv[0] << "<input_file>" << std::endl;
        return EX_USAGE;
    }
    // Read the input source file
    std::string inputFile(argv[1]);
    const auto source = readInputFile(std::string(inputFile));
    if (source.empty())
    {
        std::cout << "PPDump::Could not read input source file " << argv[1] << std::endl;
        return EX_NOINPUT;
    }

    // Parse the input
    std::string string;
    llvm::raw_string_ostream ss(string);
    bool errors = false;

    // Lex
    coral66::PPSourceObject tokens = coral66::lexer::tokenize(source, &ss, &errors);
    if (errors)
    {
        std::cout << "PPDump::Error tokenizing source file " << ss.str() << std::endl;
        return EX_DATAERR;
    }
    // Preprocess
    auto result = coral66::pp::preprocess(std::move(tokens), &ss, &errors);

    // Reconstruct
    const auto output =
        coral66::pp::reconstruct(result.data().data(), result.data().data() + result.data().size(), result);

    // Dump the preprocessed text into the output file
    generateOutputFile(inputFile, output);

    return EX_OK;
}
