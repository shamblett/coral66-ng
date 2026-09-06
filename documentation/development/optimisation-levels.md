Generally, yes, setting a higher optimization level (like -O2 or -O3) will usually result in a smaller executable size compared to no optimization, but the primary
goal of those flags is execution speed, not size.

Here is exactly how the optimization levels affect your compiled Coral66 object files (based on tests with your compiler):

1. No Optimization (Default):
   Without any -O flag, the compiler does no optimization passes on the LLVM IR. The resulting object file contains all the verbose instructions exactly as they were
   translated from your AST. This results in the largest file size.
    * In a quick test, the unoptimized object file size was 1256 bytes.

2. -O1, -O2, -O3 (Speed Optimizations):
   When you enable these, LLVM's Pass Manager aggressively simplifies the IR. It performs dead code elimination, constant folding, and inlining. Because it removes
   unnecessary instructions, the object file size shrinks significantly compared to the unoptimized version.
    * In a quick test, the -O1 and -O3 object file sizes were identical at 880 bytes.

A Nuance: -O3 vs -O2 vs Size
While -O3 shrinks the file drastically compared to no optimization, it does not always produce a smaller file than -O2.

-O3 enables aggressive loop unrolling and function inlining to make the code run as fast as mathematically possible. This can actually lead to larger file sizes than
-O2 because loops are duplicated to avoid jump overhead, and function bodies are pasted into multiple places.

If Size is Your Primary Goal
If you are compiling for a memory-constrained embedded system (baremetal) and your primary goal is the absolute smallest executable size, you would typically use -Os
(Optimize for Size) or -Oz (Optimize aggressively for size).

Currently, your action_executor.cpp only maps -O1, -O2, and -O3 to LLVM's speed optimization levels. If you wanted to support size optimization, you could easily add
a flag that maps to llvm::OptimizationLevel::Os or Oz in your pass builder.
