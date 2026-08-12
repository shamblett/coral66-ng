# Communicator specification for `coral66-ng`

1. The COMMON communicator will be supported as per the specification. The specific implementation of this for
   `coral66-ng` is as specified in the document [common-implementation.md](common/common-implementation.md)


2. The LIBRARY communicator will be supported as specified in the
   document [library-specification](library/library-specification.md).


3. The EXTERNAL communicator will be supported as specified in the
   document [external-specification](external/external-specification.md).


4. The ABSOLUTE communicator will be supported as specified in the
   document [absolute-specification](absolute/absolute-specification.md).
   Important Design Findings

# General points - see the library file libc_string.cor

   1. Namespace Collisions: The coral66-ng compiler performs strict namespace checks. You cannot have a procedure named memcpy in the EXTERNAL block and also in the
      COMMON block simultaneously, as it throws a Redefinition of symbol 'memcpy' error.
   2. Wrapper Procedures: To resolve the collision, I suffixed the EXTERNAL C-ABI imports with p (e.g., memcpyp), and exported the clean, standard names in the COMMON
      block (e.g., memcpy). The BEGIN / END block then implements short wrapper procedures that map the exported names to the external functions.
   3. Pointers as Integers: In Coral 66, all pointers (void *, char *) and sizes (size_t) are mapped to INTEGER. The language does not have a native concept of C-style
      pointers.

