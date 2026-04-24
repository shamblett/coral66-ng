Recommended Alternative: External Traversal

Instead of making the AST nodes know how to serialize themselves, you should write an external utility that knows how to
read the AST and generate JSON.

Looking at the codebase, coral66-ng already uses dynamic_cast heavily for AST traversal (for example, in
lib/frontend/semantics/semantic_analysis.cpp). You can
leverage this existing architectural pattern to build your utility entirely separated from the production code.

You could create a testing utility (e.g., test/frontend/parser/ast_json_dumper.cpp) that looks something like this:

```
    1 #include "lib/frontend/syntax/syntax.hpp"
    2 // Include your JSON library of choice here, NOT in syntax.hpp
    3 #include <rapidjson/document.h> 
    4
    5 rapidjson::Value dumpNode(const coral66::syntax::Node* node, rapidjson::Document::AllocatorType& allocator) {
    6     rapidjson::Value result(rapidjson::kObjectType);
    7
    8     if (auto* varDecl = dynamic_cast<const coral66::syntax::VariableDeclaration*>(node)) {
    9         result.AddMember("type", "VariableDeclaration", allocator);
10         // ... extract fields from varDecl and add them to result
11     }
12     else if (auto* procDecl = dynamic_cast<const coral66::syntax::ProcedureDeclaration*>(node)) {
13         result.AddMember("type", "ProcedureDeclaration", allocator);
14         // ... recursively call dumpNode on parameters and body
15     }
16     // ... handle other Node types
17
18     return result;
19 }
```

Benefits of this approach:

* Zero impact on production code: syntax.hpp remains untouched.
* Fast compiles: The heavy JSON includes are confined to a single test utility .cpp file.
* Flexibility: You can format the JSON exactly how your tests need it without worrying about exactly how the reflection
  library maps C++ members to JSON fields.

To dump the entire AST starting from the syntax::Program root, you can extend the traversal pattern into a dedicated
class. Wrapping the logic in a class is highly
recommended because rapidjson requires an allocator to construct JSON values, and a class allows you to store the
allocator as a member rather than passing it around
to every recursive call.

Here is a complete architectural blueprint for how you can build this utility in your test/ directory.

1. The Dumper Class Structure
   Create a class that contains overloaded dump() methods for the top-level structured nodes (Program, Block) and a
   dispatcher method for generic Node pointers (where
   you'll use dynamic_cast for statements, expressions, and declarations).

```
   1 #include "lib/frontend/syntax/syntax.hpp"
   2 #include <rapidjson/document.h>
   3 #include <rapidjson/stringbuffer.h>
   4 #include <rapidjson/writer.h>
   5
   6 class AstJsonDumper {
   7     // RapidJSON requires this allocator to build DOM elements
   8     rapidjson::Document::AllocatorType& m_alloc;
   9
   10 public:
   11     explicit AstJsonDumper(rapidjson::Document::AllocatorType& allocator)
   12         : m_alloc(allocator) {}
   13
   14     // 1. Entry point for the whole program
   15     rapidjson::Value dump(const coral66::syntax::Program& program) {
   16         rapidjson::Value obj(rapidjson::kObjectType);
   17         obj.AddMember("NodeType", "Program", m_alloc);
   18         
   19         // Dump Communicator (if it exists)
   20         if (const auto* comm = program.getCommunicator()) {
   21             obj.AddMember("Communicator", dump(comm), m_alloc);
   22         } else {
   23             obj.AddMember("Communicator", rapidjson::Value(rapidjson::kNullType), m_alloc);
   24         }
   25
   26         // Dump Block (The main body of the program)
   27         obj.AddMember("Block", dump(program.getBlock()), m_alloc);
   28         
   29         return obj;
   30     }
   31
   32     // 2. Dump a Block (handles vectors of unique_ptrs)
   33     rapidjson::Value dump(const coral66::syntax::Block& block) {
   34         rapidjson::Value obj(rapidjson::kObjectType);
   35         obj.AddMember("NodeType", "Block", m_alloc);
   36
   37         // Dump Declarations array
   38         rapidjson::Value decls(rapidjson::kArrayType);
   39         for (const auto& decl : block.getDeclarations()) {
   40             decls.PushBack(dump(decl.get()), m_alloc);
   41         }
   42         obj.AddMember("Declarations", decls, m_alloc);
   43
   44         // Dump Statements array
   45         rapidjson::Value stmts(rapidjson::kArrayType);
   46         for (const auto& stmt : block.getStatements()) {
   47             stmts.PushBack(dump(stmt.get()), m_alloc);
   48         }
   49         obj.AddMember("Statements", stmts, m_alloc);
   50
   51         // Dump Place References array
   52         rapidjson::Value places(rapidjson::kArrayType);
   53         for (const auto& place : block.getPlaceReferences()) {
   54             places.PushBack(dump(place.get()), m_alloc);
   55         }
   56         obj.AddMember("PlaceReferences", places, m_alloc);
   57
   58         return obj;
   59     }
   60
   61     // 3. The Dispatcher (handles polymorphic Nodes)
   62     rapidjson::Value dump(const coral66::syntax::Node* node) {
   63         if (!node) return rapidjson::Value(rapidjson::kNullType);
   64         
   65         rapidjson::Value obj(rapidjson::kObjectType);
   66
   67         // --- Declarations ---
   68         if (auto* varDecl = dynamic_cast<const coral66::syntax::VariableDeclaration*>(node)) {
   69             obj.AddMember("NodeType", "VariableDeclaration", m_alloc);
   70             // Example of dumping an identifier token
   71             // obj.AddMember("TypeToken", getTokenString(varDecl->getType()), m_alloc);
   72         }
   73         else if (auto* procDecl = dynamic_cast<const coral66::syntax::ProcedureDeclaration*>(node)) {
   74             obj.AddMember("NodeType", "ProcedureDeclaration", m_alloc);
   75             obj.AddMember("IsRecursive", procDecl->isRecursive(), m_alloc);
   76             
   77             rapidjson::Value params(rapidjson::kArrayType);
   78             for (const auto& param : procDecl->getParameters()) {
   79                 params.PushBack(dump(param.get()), m_alloc);
   80             }
   81             obj.AddMember("Parameters", params, m_alloc);
   82             
   83             // Recursively dump the procedure body statement/block
   84             obj.AddMember("Body", dump(&procDecl->getBody()), m_alloc);
   85         }
   86         // --- Statements ---
   87         else if (auto* assignStmt = dynamic_cast<const coral66::syntax::AssignmentStatement*>(node)) { // Assuming this exists based on semantic_analysis.cpp
   88             obj.AddMember("NodeType", "AssignmentStatement", m_alloc);
   89             // obj.AddMember("LHS", dump(assignStmt->getLHS()), m_alloc);
   90             // obj.AddMember("RHS", dump(assignStmt->getRHS()), m_alloc);
   91         }
   92         // --- Expressions ---
   93         else if (auto* binExpr = dynamic_cast<const coral66::syntax::BinaryExpression*>(node)) { // Assuming this exists based on semantic_analysis.cpp
   94             obj.AddMember("NodeType", "BinaryExpression", m_alloc);
   95             // obj.AddMember("Left", dump(binExpr->getLeft()), m_alloc);
   96             // obj.AddMember("Right", dump(binExpr->getRight()), m_alloc);
   97         }
   98         // ... Add cases for all other AST nodes ...
   99         else {
   100             obj.AddMember("NodeType", "UnknownNode", m_alloc);
   101         }
   102
   103         return obj;
   104     }
   105
   106 private:
   107     // Helper function to extract text from a Lexer CTokenIterator
   108     rapidjson::Value getTokenString(const coral66::lexer::CTokenIterator& token) {
   109         // You would extract the lexeme/string from your token iterator here.
   110         // Assuming something like token->getSpelling():
   111         // std::string text = token->getSpelling();
   112         // return rapidjson::Value(text.c_str(), m_alloc);
   113         return rapidjson::Value("dummy_token", m_alloc);
   114     }
   115 };
```

2. Using the Dumper to output JSON
   To actually use the AstJsonDumper in a test or a CLI application, you initialize a rapidjson::Document, pass its
   allocator to your dumper, format it as a string, and
   output it.

```
    1 #include <iostream>
    2
    3 void printAstAsJson(const coral66::syntax::Program& program) {
    4     // 1. Create the RapidJSON Document
    5     rapidjson::Document doc;
    6     doc.SetObject();
    7
    8     // 2. Instantiate our dumper, providing the document's allocator
    9     AstJsonDumper dumper(doc.GetAllocator());
10
11     // 3. Dump the AST into the RapidJSON document root
12     rapidjson::Value root = dumper.dump(program);
13     doc.Swap(root);
14
15     // 4. Serialize to string for printing
16     rapidjson::StringBuffer buffer;
17     
18     // Use PrettyWriter instead of Writer for human-readable output
19     rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
20     doc.Accept(writer);
21
22     // 5. Print to console (or write to a test artifact file)
23     std::cout << buffer.GetString() << std::endl;
24 }
```

Key Takeaways:

1. JSON Arrays from Vectors: Program and Block contain std::vector<std::unique_ptr<T>>. To serialize these, you iterate
   over them, unwrap the unique pointer with
   .get(), pass it back into the polymorphic dump(Node*) dispatcher, and push the result into a rapidjson::kArrayType.
2. Dynamic Dispatching: Because Declaration, Statement, and Expression nodes are stored polymorphically as pointers in
   the tree, a single dump(const Node*) function
   equipped with an if / else if chain of dynamic_casts acts as the router to safely unpack every subtype.
3. Tokens: When extracting names, types, and values, you will likely encounter lexer::CTokenIterator variables (like in
   VariableDeclaration::getType()). You will
   need a small helper to extract the actual lexeme string from those iterators and convert them into RapidJSON strings
   so your JSON dump has the actual
   human-readable variable names and operator types.

To correctly extract the text or value from a CTokenIterator (which in coral66-ng is an alias for const coral66::lexer::
CToken*), we need to look at how CToken is
implemented in your codebase.

Based on the headers in lib/frontend/common/lexer_common.hpp, a CToken derives from TokenBase and contains two important
pieces of information:

1. The TokenType (e.g., Identifier, Integer, Plus, Assignment), which can be converted to a string using the existing
   coral66::lexer::tokenName() utility.
2. A parsed Value stored as a std::variant (which can hold std::string for identifiers, or LLVM types like llvm::APSInt
   and llvm::APFloat for numbers).

Here is how you can implement the helper method inside the AstJsonDumper class. It uses std::visit to crack open the
token's internal variant and extracts the most
accurate string representation:

```
    1 #include "lib/frontend/common/lexer_common.hpp"
    2 #include <rapidjson/document.h>
    3 #include <llvm/ADT/SmallString.h>
    4
    5 // Helper method to add inside your AstJsonDumper class
    6 rapidjson::Value AstJsonDumper::getTokenString(coral66::lexer::CTokenIterator token) {
    7     if (!token) {
    8         return rapidjson::Value(rapidjson::kNullType);
    9     }
10
11     // 1. Get the base token name (e.g. "Identifier", "+", "Integer")
12     std::string_view tokenNameView = coral66::lexer::tokenName(token->getTokenType(), false);
13     std::string tokenName(tokenNameView);
14
15     // 2. Extract the actual parsed value from the token's variant payload
16     std::string tokenValueStr;
17     const auto& valueVariant = token->getValue();
18
19     std::visit([&](auto&& arg) {
20         using T = std::decay_t<decltype(arg)>;
21         
22         if constexpr (std::is_same_v<T, std::string>) {
23             // Usually identifiers or string literals
24             tokenValueStr = arg;
25         }
26         else if constexpr (std::is_same_v<T, coral66::lexer::NonCharString>) {
27             // Specialized internal string representation
28             tokenValueStr = "<NonCharString>";
29         }
30         else if constexpr (std::is_same_v<T, llvm::APSInt>) {
31             // Integer literals
32             llvm::SmallString<32> str;
33             arg.toString(str);
34             tokenValueStr = str.c_str();
35         }
36         else if constexpr (std::is_same_v<T, llvm::APFloat>) {
37             // Floating point literals
38             llvm::SmallString<32> str;
39             arg.toString(str);
40             tokenValueStr = str.c_str();
41         }
42         else if constexpr (std::is_same_v<T, std::monostate>) {
43             // Token doesn't carry a payload (e.g., standard operators like '+' or ':=')
44             // In this case, the token name/symbol is the value itself.
45             tokenValueStr = tokenName;
46         }
47     }, valueVariant);
48
49     // 3. Format it beautifully as a JSON Object for the dump
50     // Example output: { "Class": "Identifier", "Value": "myVariable" }
51     //                 { "Class": "Assignment", "Value": ":=" }
52     
53     rapidjson::Value tokenObj(rapidjson::kObjectType);
54     
55     // Allocate strings using RapidJSON's allocator so they live inside the Document
56     rapidjson::Value nameVal;
57     nameVal.SetString(tokenName.c_str(), static_cast<rapidjson::SizeType>(tokenName.length()), m_alloc);
58     
59     rapidjson::Value valVal;
60     valVal.SetString(tokenValueStr.c_str(), static_cast<rapidjson::SizeType>(tokenValueStr.length()), m_alloc);
61
62     tokenObj.AddMember("Class", nameVal, m_alloc);
63     tokenObj.AddMember("Value", valVal, m_alloc);
64
65     return tokenObj;
66 }
```

How to use it in your AST Node dumping logic:

If you are dumping a VariableDeclaration, for example, you can now easily dump its type and the identifiers it declares:

```
    1 // Inside AstJsonDumper::dump(const coral66::syntax::Node* node)
    2
    3 if (auto* varDecl = dynamic_cast<const coral66::syntax::VariableDeclaration*>(node)) {
    4     obj.AddMember("NodeType", "VariableDeclaration", m_alloc);
    5     
    6     // Dump the 'Type' keyword token (e.g., INTEGER, FLOATING, FIXED)
    7     obj.AddMember("VarType", getTokenString(varDecl->getType()), m_alloc);
    8
    9     // Dump all identifiers declared in this statement
10     rapidjson::Value identifiers(rapidjson::kArrayType);
11     for (coral66::lexer::CTokenIterator idToken : varDecl->getIdentifiers()) {
12         identifiers.PushBack(getTokenString(idToken), m_alloc);
13     }
14     obj.AddMember("Identifiers", identifiers, m_alloc);
15 }
```
