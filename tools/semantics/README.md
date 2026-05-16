# Semantics Program Dump tool

This package provides a dump tool that dumps the program context output of a semantically analysed source program.

It is intended to be used as a general fault investigation tool and as a test tool in semantics golden testing.

It consists of a program dump library and a binary file to allow the program dump to be called from the command line.

The binary, named ```programdump``` expects only one parameter, the absolute path of the Coral 66 source file to dump, it can be
invoked as such :-

```
bazel run //tools/semantics/programdump/bin:programdump -- /home/steve/Development/c++/coral/coral66-ng/test/examples/benchmark/ackerman.cor
```

The output is a JSON formatted file comprising the following sections :-

* The symbol table.
* Resolved identifiers
* Expression types
* Usables(Resolve symbols)
* Auxiliary types
* Table information
* Errors and Diagnostics


The tool uses the [RapidJSON](https://rapidjson.org/) library to generate the JSON output.