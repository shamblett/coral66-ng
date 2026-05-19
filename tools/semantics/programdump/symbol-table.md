In the symbol table dump of the programdump utility I wish to only dump the names of symbols encountered in the source code alomg with their scope number(depth).   
How would this be best achieved using the context members detailed above?     

To achieve this using the context members, you must rely on context.ownedUsables. This vector acts as the permanent ledger of every symbol encountered and created
during the pass.

However, because the SymbolTable scope stack is transient (it's empty by the end), the scope depth must be captured at the exact moment the symbol is created.
Currently, semantics::Declaration does not store this information.

Here is the best architectural approach to achieve this:

1. Add Scope Depth to Declaration
   Update the semantics::Declaration base class in lib/frontend/semantics/semantics.hpp to store the scope depth:

    1 class Declaration : public Usable
    2 {
    3     lexer::CTokenIterator m_nameToken;
    4     const syntax::Declaration* m_astNode = nullptr;
    5     int m_scopeDepth = 0; // <--- Add this
    6
    7 protected:
    8     template <class T>
    9     Declaration(std::in_place_type_t<T>, const lexer::CTokenIterator nameToken)
10         : Usable(std::in_place_type<T>), m_nameToken(nameToken)
11     {
12     }
13
14 public:
15     // ... existing methods ...
16     
17     void setScopeDepth(int depth) { m_scopeDepth = depth; }
18     [[nodiscard]] int getScopeDepth() const { return m_scopeDepth; }
19 };

2. Capture the Scope in SymbolResolutionPass
   Whenever SymbolResolutionPass.cpp creates a new declaration using makeIntrusiveVariant<...Declaration>, immediately query the current scope depth from the symbol
   table and set it before pushing it to ownedUsables:

1 auto declaration = makeIntrusiveVariant<VariableDeclaration>(id, std::move(typeCopy));
2 declaration->setScopeDepth(m_context.symbolTable.scopeCount()); // <--- Capture current depth
3 m_context.symbolTable.insert(name, declaration.get());
4 m_context.ownedUsables.push_back(std::move(declaration));

3. Dump ownedUsables in ProgramDump
   Finally, replace the call to dump(symbolTable) in program_dump.cpp. You can iterate directly over program.getContext().ownedUsables to build your JSON. Since every
   subclass of Usable inherits from Declaration, you can safely check for it using the matching pattern we discussed earlier:

    1 rapidjson::Value ProgramDump::dump(const semantics::Program& program)
    2 {
    3     rapidjson::Value obj(rapidjson::kObjectType);
    4     rapidjson::Value symbols(rapidjson::kArrayType);
    5
    6     for (const auto& usablePtr : program.getContext().ownedUsables)
    7     {
    8         usablePtr->match([&](const auto& decl) {
    9             // Ensure we are dealing with a Declaration to safely call getScopeDepth and getName
10             if constexpr (std::is_base_of_v<semantics::Declaration, std::decay_t<decltype(decl)>>)
11             {
12                 rapidjson::Value symbolObj(rapidjson::kObjectType);
13                 symbolObj.AddMember("Name", rapidjson::StringRef(decl.getName().data()), m_alloc);
14                 symbolObj.AddMember("Scope Depth", decl.getScopeDepth(), m_alloc);
15                 symbols.PushBack(symbolObj, m_alloc);
16             }
17         });
18     }
19
20     obj.AddMember("Symbols", symbols, m_alloc);
21     return obj;
22 }

This approach cleanly separates the transient symbol table resolution rules from the permanent graph of semantic entities without requiring heavy changes to the
compiler's architecture.
