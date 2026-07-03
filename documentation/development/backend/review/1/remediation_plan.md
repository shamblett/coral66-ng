# Backend Code Generation Remediation Plan

This document outlines the step-by-step remediation plan to address the remaining deficiencies identified in the compliance review (`review.md`). The plan is structured around the architectural phases of the backend code generator.

*(Note: Array lower bound offset calculation and `OVERLAY` aliasing issues from Phase 6 have already been successfully remediated and tested.)*

## Phase 3: Data Types & Part-Word Access

### 1. FIXED Point Type Scaling
**Problem:** FIXED types (`FIXED(total, fraction)`) currently operate identically to integers. They lack the required shifting and scaling logic during arithmetic operations.
**Remediation Steps:**
1.  **Type Promotion & Casting (`lib/backend/code_generator_expressions.cpp`)**: Update the `visit(BinaryExpression)` and assignment statement logic to inspect operands for `semantics::FixedType`.
2.  **Fractional Alignment**: If two `FIXED` operands have different fractional bits, emit `llvm::Instruction::Shl` or `llvm::Instruction::AShr` instructions to align their binary points before executing arithmetic operations (`CreateAdd`, `CreateSub`, etc.).
3.  **Multiplication/Division Scaling**: Following a multiplication (`CreateMul`), shift the result right by the fractional bits to correct the scale. Following a division (`CreateSDiv`), shift the numerator left by the fractional bits *before* the division.
4.  **Testing (`test/backend/data/p3/`)**: Create comprehensive fixed-point execution tests verifying accuracy and scale bounds across arithmetic boundaries.

### 2. BITS Part-Word Access
**Problem:** Native bit-field extraction for part-word access is unimplemented.
**Remediation Steps:**
1.  **Implement `visit(PartWordExpression)` (`lib/backend/code_generator_expressions.cpp`)**:
    - Emit the address of the base expression (`emitAddress`).
    - Load the underlying value (`CreateLoad`).
    - Calculate the bit offset using the start bit expression (`CreateLShr`).
    - Create a bitmask based on the width expression `((1 << width) - 1)` and apply it (`CreateAnd`) to extract the final value.
2.  **Implement Part-Word Assignment (`lib/backend/code_generator_statements.cpp`)**: Update `visit(AssignmentStatement)` to handle `PartWordAccess` on the LHS.
    - Load the current value of the base target.
    - Construct an inverse bitmask `~(((1 << width) - 1) << startBit)` to clear the destination bits.
    - Shift and mask the new value to fit the target field.
    - `CreateOr` the cleared base and the shifted new value, and `CreateStore` it back to the base address.

## Phase 4 & 5: Procedures and Advanced Control Flow

### 3. LABEL and SWITCH Parameters
**Problem:** Passing Labels and Switches as procedure parameters is unimplemented.
**Remediation Steps:**
1.  **Type Conversion (`lib/backend/code_generator.cpp`)**: Update `convertType` to handle `semantics::LabelType` and `semantics::SwitchType`. For LLVM, these should be lowered to `llvm::PointerType::getUnqual(m_module.getContext())` (opaque pointers pointing to block addresses).
2.  **Argument Emission (`lib/backend/code_generator_expressions.cpp`)**: Update procedure call argument generation. If an argument is a label, use `llvm::BlockAddress::get(m_currentFunction, targetBlock)` to yield a value.
3.  **Indirect Branching (`lib/backend/code_generator_statements.cpp`)**: Update `GOTO` statements to handle dynamic pointers. If the target is an evaluated label parameter, emit an `llvm::IndirectBrInst` instead of a static `llvm::BranchInst`. Note that `llvm::IndirectBrInst` requires explicit definition of all possible target blocks in the procedure scope.

## Phase 6: Complex Data Structures & Initialization

### 4. TABLE Bit-Level Packing
**Problem:** TABLEs currently use standard C-style structs and ignore developer-defined field allocations (width and bit offsets).
**Remediation Steps:**
1.  **Structure Definition (`lib/backend/code_generator.cpp`)**: Modify `convertType` for `semantics::TableDeclaration`. Rather than creating a packed struct of individual elements, a Coral 66 TABLE should be represented as an array of `i32` words (matching the target word size), sized by the total `Width * Length` defined in the specification.
2.  **Element Access (`lib/backend/code_generator_expressions.cpp`)**: Implement element extraction when a `TableElementDeclaration` is accessed.
    - Calculate the word offset using the array index and the table's defined width per entry.
    - Extract the part-word value dynamically using the element's defined bit position and length. (This logic will heavily reuse the logic implemented for BITS Part-Word Access).
3.  **Element Assignment (`lib/backend/code_generator_statements.cpp`)**: Update LHS assignment for table elements using the same inverse-masking technique required for part-word assignments.

### 5. PRESET Data Initialization
**Problem:** `PRESET` is completely unsupported, meaning tables and arrays cannot be statically initialized at compile time.
**Remediation Steps:**
1.  **Global Variable Initialization (`lib/backend/code_generator_declarations.cpp`)**: Modify the global instantiation logic. Instead of passing `llvm::Constant::getNullValue(llvmType)`, analyze the `semantics::ArrayDeclaration` or `semantics::TableDeclaration` for associated presets.
2.  **Constant Array Construction**: If presets exist, map the parsed literal values into a corresponding `std::vector<llvm::Constant*>` and create an `llvm::ConstantArray::get(...)` or `llvm::ConstantStruct::get(...)` to initialize the global variable.
3.  **Local PRESET Support (Optional but Recommended)**: Since Coral 66 restricts PRESET to anonymous or global storage, verify if local preset generation needs to hoist data into `.rodata` and issue `llvm::MemCpyInst` to populate the local stack variable at function entry.

## Phase 7: Communicators

### 6. COMMON and ABSOLUTE Specifications
**Problem:** `COMMON` blocks and `ABSOLUTE` variables are entirely missing.
**Remediation Steps:**
1.  **COMMON Blocks**: For variables declared within a `COMMON` communicator, they must be emitted as `llvm::GlobalVariable`s with `llvm::GlobalValue::CommonLinkage` or `ExternalLinkage` with a specific `.bss` section name to guarantee the linker merges identical definitions across compilation units.
2.  **ABSOLUTE Variables**: Hardware mapped variables. The code generator must treat `ABSOLUTE` variables as hardcoded integer pointers (`llvm::ConstantExpr::getIntToPtr`) pointing directly to the specified memory mapped address, rather than emitting a global storage allocation.


### Phased implementation

This document details the following remaining tasks mapped out to their respective codebase sections:
* Phase 3: Fractional shifting and scaling operations for FIXED point variables, and exact bitmask extraction techniques for BITS part-word data access.
* Phase 4/5: Lowering LABEL and SWITCH parameters to llvm::BlockAddress structures and using llvm::IndirectBrInst for dynamic branching inside procedures.
* Phase 6: Defining TABLE memory mappings as raw word arrays instead of C-style structs to guarantee tight control over bit-width boundaries, and evaluating PRESET
  expressions to generate statically initialized .rodata equivalents using llvm::ConstantArray during global variable instantiation. 