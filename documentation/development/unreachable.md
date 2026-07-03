While your premise is intuitively correct—code that can never be reached under any circumstances shouldn't need to exist—macros like CORAL66_UNREACHABLE (which typically
wraps LLVM's llvm_unreachable or compiler intrinsics like __builtin_unreachable()) serve a very specific and necessary purpose in modern C++ codebases.


The core issue is the gap between what the programmer logically knows to be true and what the compiler can mathematically prove.

Here are the specific reasons why this macro is used, along with its pros and cons.


1. Exhaustive Switch Statements (The Most Common Use Case)
   In C++, an enum class has an underlying integer type. Even if you cover every single enum value in a switch statement, the compiler knows that theoretically, the memory
   could hold an invalid integer value (e.g., via a bad cast or memory corruption).


    1 enum class Token { Plus, Minus };
    2
    3 int process(Token t) {
    4     switch(t) {
    5         case Token::Plus: return 1;
    6         case Token::Minus: return 2;
    7     }
    8     // The compiler will throw a warning here: "control reaches end of non-void function"
    9     CORAL66_UNREACHABLE;
10 }
Why not just use a default case?
If you write default: return 0;, you silence the missing return warning. However, you also destroy the compiler's ability to warn you if you forget to handle a new enum
value. By omitting the default case, compilers (via -Wswitch) will warn you if you add Token::Asterisk but forget to update this switch statement. The UNREACHABLE macro
at the end satisfies the missing-return warning while preserving the exhaustive-switch warning.


2. Compiler Optimization
   When you use an unreachable macro, it resolves to intrinsic compiler instructions (like __builtin_unreachable() in GCC/Clang or __assume(0) in MSVC).
   This tells the compiler's optimizer: "Assume control flow will literally never reach this point."
   Armed with this guarantee, the compiler can drastically optimize the generated machine code by:
* Removing branch predictions for that path.
* Avoiding generating a function epilogue (like ret instructions).
* Packing the code tighter, which improves CPU instruction cache performance.


3. Defensive Programming & Debugging
   In a well-implemented codebase, UNREACHABLE macros behave differently depending on the build type:
* Release Build: It acts as an optimization hint (__builtin_unreachable()).
* Debug Build: It acts as an assertion (abort() or assert(false)).
  If a memory corruption event or a logical bug does cause the program to reach that state, a Debug build will instantly crash and print a stack trace exactly at the
  impossible state, rather than silently falling through into undefined behavior.

  ---


Pros of using CORAL66_UNREACHABLE
* Preserves -Wswitch warnings: Allows developers to get compiler errors when they forget to handle new enum types.
* Better Performance: Gives the optimizer permission to strip out dead code paths and avoid unnecessary branch evaluations.
* Documentation of Intent: It explicitly tells other developers reading the code, "I know execution could theoretically fall through here based on the types, but the
  system invariants guarantee it won't."
* Fail-fast in Debugging: Catches impossible states during development rather than letting them cause subtle data corruption.


Cons of using CORAL66_UNREACHABLE
* Catastrophic Undefined Behavior if Reached: If the programmer is wrong and the code is somehow reached in a Release build, it triggers Undefined Behavior (UB). The
  compiler, having optimized the code under the assumption it would never be reached, will execute whatever garbage machine code happens to be next in memory. This can
  lead to the most difficult-to-debug crashes possible.
* Can Be Used as a Crutch: Sometimes developers use it to silence compiler warnings when they really should be handling an edge case or refactoring their types (e.g.,
  using std::variant or std::optional instead of passing around invalid state).


Summary
You are completely right that in a perfectly typed, mathematically provable language, UNREACHABLE shouldn't be necessary. But in C++, the type system isn't strict enough
to express every invariant to the compiler. CORAL66_UNREACHABLE is the tool developers use to bridge that gap safely and efficiently.
