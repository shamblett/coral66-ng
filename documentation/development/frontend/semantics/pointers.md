
There are many visit calls in the semantics code that use raw pointers, an example is line 107 of lib/frontend/semantics/symbol_resolution_pass.cpp. Isnt this      
frowned upon in modern C++? Shouldn't these calls either be references or smart pointers e.g. unique_ptr?

In modern C++, the use of raw pointers is indeed frowned upon when they are used for resource management or ownership. However, using raw pointers as non-owning
observers—which is what is happening here—is completely standard, safe, and idiomatic.

Let's break down exactly why the code at line 107 is written this way and why references or smart pointers wouldn't work for this specific pattern:

1 void SymbolResolutionPass::visit(const syntax::Statement& node)
2 {
3     if (auto* blockStmt = dynamic_cast<const syntax::BlockStatement*>(&node))
4         visit(*blockStmt);
5     // ...

1. Why not use References?
   If you used a reference with dynamic_cast, the code would look like this:

1 auto& blockStmt = dynamic_cast<const syntax::BlockStatement&>(node);
The problem is how dynamic_cast reports failure.
- When casting pointers, a failed cast returns nullptr. This is extremely fast and allows the if (auto* ptr = ...) syntax to cleanly evaluate to false.
- When casting references, a failed cast throws a std::bad_cast exception.

Because this code is essentially a "type switch" (evaluating an if / else if chain to find out which specific subclass of Statement the node is), the cast is
expected to fail most of the time until it hits the right type. Using references would mean throwing and catching exceptions for standard control flow, which is
severely detrimental to performance and considered an anti-pattern.

2. Why not use Smart Pointers (std::unique_ptr / std::shared_ptr)?
   Smart pointers enforce ownership.
- A std::unique_ptr implies "I exclusively own this memory and am responsible for deleting it."
- A std::shared_ptr implies "I share ownership of this memory."

The visit function doesn't own the AST nodes; it is simply navigating through an Abstract Syntax Tree that is stored and managed elsewhere. The raw pointer returned
by dynamic_cast<...*>(&node) is just temporary, non-owning "view" of the object. Taking an existing object and arbitrarily wrapping it in a smart pointer
mid-function would incorrectly attempt to take ownership of it, eventually causing a double-free crash when the smart pointer went out of scope.

Summary
The code here perfectly mixes references and pointers for their intended modern purposes:
1. The function takes the polymorphic object by reference (& node) because it expects a valid, existing object.
2. It takes the address (&node) and uses a raw pointer with dynamic_cast strictly to leverage the non-throwing nullptr check.
3. Once the type is successfully identified, it immediately dereferences the pointer visit(*blockStmt) to pass it back as a reference to the overloaded subclass
   visitor.

