This directory contains machine generated Coral 66 source files to support golden testing of the lexer, preprocessor and
parser
processing chain. Each generated file must conform to the following specification :-

- The source files must have an extension of .cor
- Stropped or unstropped keywords can be used but not in the same source file, i.e. all keywords in the same source file
  must be stropped or unstropped
- The first statement in the source file must be a valid PROGRAM or SEGMENT declaration, e.g ```PROGRAM test1```
- If a communicator is generated it must be a COMMON communicator in between the PROGRAM or SEGMENT declaration and the
  first BEGIN,
- Identifiers should be lower camel case
- Comments whether single line, multi line or end of line can be used anywhere in the source file after the initial
  PROGRAM or SEGMENT declaration. 