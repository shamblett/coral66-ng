# Parser ASTDump tool

This package provides a dump tool that dumps a program AST as produced by the parser to a JSON formatted output file.

It is intended to be used as a general fault investigation tool and as a test tool in parser golden testing.

It consists of an AST dump library and a binary file to allow the AST dump to be called from the command line.

The binary, named astdump expects only one parameter, the absolute path of the Coral 66 source file to dump, it can be
invoked as such :-

```
bazel run //tools/parser/astdump/bin:astdump -- /home/steve/Development/c++/coral/coral66-ng/test/examples/benchmark/ackerman.cor
```

The tool uses the [RapidJSON](https://rapidjson.org/) library to generate the JSON output.