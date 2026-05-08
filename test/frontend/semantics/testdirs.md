1. Can you use $(location) on the existing testdata filegroup?

No, it is not possible to use $(location) on the existing testdata target on line 8. Because testdata uses a glob(["data/benchmark/*"]), it expands to multiple file
targets. The $(location) Make variable expects exactly one file or label, so Bazel will throw a $(location) evaluates to multiple files error.

  ---

2. How to do it using Make Variables

If you specifically want to resolve the directory dynamically within the BUILD file, you must declare a separate target that points directly to the directory itself.
Furthermore, for test execution environments, you should prefer $(rootpath) over $(location), as it guarantees the correct path relative to the test's runfiles tree.

You can accomplish this by updating your BUILD file:

    1 # Create a new filegroup targeting the directory itself
    2 filegroup(
    3     name = "benchmark_dir",
    4     srcs = ["data/benchmark"],
    5 )
    6
    7 # Then use $(rootpath) in the cc_test env attribute
    8 cc_test(
    9     name = "semantics",
10     srcs = [
11         "main.cpp",
12     ],
13     data = [
14         ":benchmark_dir", # Ensure the directory is available in runfiles
15     ],
16     env = {
17         "BENCHMARK_DIR": "$(rootpath :benchmark_dir)",
18     },
19     visibility = ["//test:__pkg__"],
20     deps = [
21         "semanticstest",
22     ],
23 )

  ---

3. Simpler Alternatives

Alternative A: Hardcode the runfiles path (Standard Bazel approach)
Because Bazel enforces a completely deterministic runfiles directory layout during testing, dynamically resolving static paths using Make variables isn't strictly
necessary. You can just safely hardcode the repository-relative path in the env attribute:

1 cc_test(
2     name = "semantics",
3     # ...
4     env = {
5         "BENCHMARK_DIR": "test/frontend/semantics/data/benchmark",
6     },
7 )
