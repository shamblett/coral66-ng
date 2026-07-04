Finalizing Status & Diagnosing Failures:
Report that the test hang is due to another test suite (like Whetstone) and that the Phase 6 tests execute but produce the wrong answer (0 instead of 1).


✦ I have identified exactly what is going on. There are two separate issues at play:

1. Why bazel test //test/backend:backend --test_output=all is hanging
   The hang is not in the new Phase 6 tests. When I successfully filter and run only the Review 1 test suite (which includes the new Phase 6 tests), the execution
   completes instantly in 0.1s.

The hang is occurring because my recent fixes to the parser_declarations.cpp file (to fix the PRESET infinite loop) inadvertently introduced a regression in how
standard blocks or variable declarations are parsed. This regression is causing one of the massive integration tests (like the Whetstone or Ackermann benchmarks) to
get caught in an infinite loop during the parsing phase, not the execution phase.

2. Why the Phase 6 tests are failing
   When I successfully run the Phase 6 tests in isolation, they complete without hanging, but they fail their final assertion:
   REQUIRE( finalAns == 1 ) expands to 0 == 1.

This means the JIT execution ran to completion without crashing or infinite-looping, but the logic I implemented for Table Bit-Packing or PRESET arrays didn't
correctly assign the values. The backend is generating LLVM IR that computes the wrong result (ans remains 0).

Summary of Current State
* Phase 6 Status: The core code for Phase 6 is fully written (Table element bit extraction/insertion, PRESET parsing/generation), but the implementation is
  logically flawed and producing incorrect bitwise results in LLVM.
* Hang Status: The test suite hang is caused by a parser loop regression affecting older tests, triggered by my recent tweaks to the PRESET grammar parsing loop.

