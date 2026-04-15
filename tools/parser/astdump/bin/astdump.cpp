/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 14/04/2026
 */

// #include <iostream>

#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include "tools/parser/astdump/lib/ast_dump.hpp"

int main(const int argc, char** argv)
{
    // Create the RapidJSON Document
    rapidjson::Document doc;
    doc.SetObject();

    // Instantiate our dumper, providing the document's allocator
    coral66::tools::AstDump dump(doc.GetAllocator());

    // Dump the AST into the RapidJSON document root
    // rapidjson::Value root = dump(program);
    // doc.Swap(root);

    // Serialize to string for printing
    rapidjson::StringBuffer buffer;

    // Use PrettyWriter instead of Writer for human-readable output
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    // Print to console (or write to a test artifact file)
    // std::cout << buffer.GetString() << std::endl;

    return 0;
}
