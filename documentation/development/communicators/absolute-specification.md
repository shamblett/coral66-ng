✦ In the context of modern, user-space operating systems (like Linux), directly accessing hardcoded absolute addresses
will result in an immediate Segmentation Fault due to virtual memory protections and Address Space Layout Randomization (ASLR). For standard desktop applications,
the ABSOLUTE communicator is indeed redundant.

However, in the future `coral66-ng` may target embedded systems, microcontrollers, or bare-metal environments. In those domains, absolute
addressing is still the standard way to interact with Memory-Mapped I/O (MMIO) registers (e.g., reading from a hardware UART register at 0x4000C000)
so `coral66-ng` should be updated to support these embedded environments.


Here is a specification and implementation statement  of how the ABSOLUTE communicator in `coral66-ng` should work.

Map the absolute address directly into LLVM IR using pointer casting.

* Syntax: ABSOLUTE ( INTEGER myHardwareTimer / 4096 ); (assuming 4096 is 0x1000). The absolute value should be able to be specified 
  as a hex value as well as a decimal value.
* Semantics: The Semantic Analyzer registers myHardwareTimer as a global variable.
* Code Generation: Instead of allocating an llvm::AllocaInst or a standard llvm::GlobalVariable, you use LLVM's inttoptr instruction.
    * When the code reads myHardwareTimer, the code generator emits: inttoptr i64 4096 to i32*.
    * Crucial Modern Detail: You must mark any LLVM load or store instructions to this address as volatile. Otherwise, the aggressive LLVM optimizer will assume the
      memory hasn't changed and optimize your hardware polling loops into infinite loops!

* Introduce a compiler flag in action_executor, such as --target-env=baremetal vs --target-env=os.
* If targeting bare-metal, use LLVM inttoptr with volatile memory operations.

This will make `coral66-ng` incredibly powerful, allowing it to compile legacy avionics/military Coral 66 code for modern microcontrollers without changing the original
source code.

