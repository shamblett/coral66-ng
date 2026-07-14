# Specification compliance testing

This directory contains a suite of tests to verify compiler behaviour against the specification.

The test are arranged on a specification section by section basis, each section test having its own tests and data.
Functional language areas such as array semantics say may be split over several sections, unlike unit testing which is more
area based the test suite adheres strictly to the specification sections for traceability purposes. This means that a semantic
area may well be tested in its entirety across several compliance test.

In general the tests use the same strategy as that used in the backend tests, i.e. a Coral 66 source code file is compiled 
and JIT'd with variable values set during the test being inspected and verified for correctness. This may vary however if
needed on a test by test basis.

Each test case/section is annotated with the particular part of the specification it is verifying. Any example code
present in a section is also tested in its own test file named 'examples'. Coral 66 source code files are commented to
describe expected behaviour as appropriate.

Coral 66 source code test files are named after the relevant specification section and the test number, for instance 1-4-1.cor refers to
section 1.4 test 1, 1-4-2.cor to section 1.4 test 2 and so on.

