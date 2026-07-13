# Specification compliance testing

This directory contains a suite of tests to verify compiler behaviour against the specification.

The test are arranged on a specification section by section basis, each section test having its own tests and data.

In general the tests use the same strategy as that used in the backend tests, i.e. a Coral 66 source code file is compiled 
and JIT'd with variable values set during the test being inspected and verified for correctness. This may vary however if
needed on a section by section basis.

Each test case/section is annotated with the particular part of the specification it is verifying. Any example code
present in a section is also tested in its own test file named 'examples'. Coral 66 source code files are commented to
describe expected behaviour as appropriate.

