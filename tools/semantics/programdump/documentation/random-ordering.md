Debugging Non-Deterministic AST Map Sorting: Explaining the root cause of the non-deterministic output difference between test runs to the user.


✦ I've dug quite deep into the issue and here's exactly what's happening.

The programdump tool loops over expressionTypes and resolvedIdentifiers (both std::unordered_maps) and sorts the nodes for stable JSON output. The problem is that
there are duplicate nodes in the AST at the exact same location in the source code.

For example, look at line 56:  'ELSE''IF' n = 0 'THEN'
There are two completely distinct AST nodes representing n on that exact line with the same file ID and offset.

Because my first fix only sorted by FileId and Offset like this:

1 return std::make_tuple(p.first->begin()->getFileId(), p.first->begin()->getOffset());
...when std::ranges::sort encounters the duplicate n nodes, it sees them as "equal". Since std::ranges::sort is not a stable sort (and even if it was, the input
comes from a non-deterministic unordered map), the order of those duplicate nodes is entirely random on every run.

My second attempt was to add the raw memory pointer as a tie-breaker:

1 return std::make_tuple(..., reinterpret_cast<std::uintptr_t>(p.first));
While this technically "determines" the sort, it completely breaks the tests because the memory address changes on every single run of Bazel, resulting in a JSON
array that has completely different node orderings compared to the golden ackerman_program.json file.

To truly fix this, we need a stable property to tie-break identical nodes that doesn't rely on random memory addresses. Let me know if you know of any unique
internal IDs that the syntax::Node class provides, otherwise I can dive into the parser to try and figure out a deterministic fallback!

Why are the AST nodes duplicated?

The duplicate AST nodes are occurring because of how the parser evaluates conditional logic and procedures.

If we look at the recursive Ackermann call in the golden file: Ackermann (m - 1, Ackermann (m, n - 1))

When SymbolResolutionPass visits a ParameterDeclaration with ParameterMode::Value, it allocates a specific "value" copy of the integer type. Later, when the parser
encounters the identifier m and n in expressions, TypeCheckingPass infers their types by checking the ResolvedIdentifiers map.

Because m and n are passed by value in the Ackermann recursive call, the semantic analyzer seems to construct two separate IdentifierExpression nodes for an
identifier on the same line:
1. One represents the identifier as an lvalue/reference (e.g. looking up the variable's memory location in the scope).
2. The other represents the identifier evaluated as an rvalue/value (e.g. loading the integer value to pass into the Ackermann function call or compare it against
   0).

This is why, for the expression n = 0 on line 56, we see two entries for n in the expressionTypes JSON:

    1         {
    2             "ExpressionText": "n",
    3             "Location": { "FileId": 0, "Line": 56, "Column": 18 },
    4             "Type": {
    5                 "Name": "PrimitiveType",
    6                 "Kind": "Integer",
    7                 "IsValue": false    // <-- Lvalue
    8             }
    9         },
10         {
11             "ExpressionText": "n",
12             "Location": { "FileId": 0, "Line": 56, "Column": 18 },
13             "Type": {
14                 "Name": "PrimitiveType",
15                 "Kind": "Integer",
16                 "IsValue": true     // <-- Rvalue
17             }
18         },

Both are perfectly valid AST constructions; the semantic analyzer is essentially wrapping the lvalue identifier with an implicit "lvalue-to-rvalue" cast node or
recording both states during type checking.

Because they exist at the exact same token offset and file ID, sorting them by location alone is ambiguous. When I used reinterpret_cast<std::uintptr_t> as a
tie-breaker, the sort order became dependent on the heap allocator, causing the JSON array order to flip-flop between runs, failing the golden test.

To fix this deterministically, we need to tie-break on a property of the AST node itself that doesn't change between runs. A good candidate for this is the Type's
IsValue flag, or the raw text of the dump itself, or even the type of the Usable declaration.

