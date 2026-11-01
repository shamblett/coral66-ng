# Building and installing

## Building

There are three ways to build the compiler, locally, using a developer podman script and using a user podman script.
The podman scripts obviously need podman installed on your platform, the other options each have their own dependencies,
see below.
The project was originally built on a Fedora 44 platform. This comes with LLVM 22 as standard. You can use lower LLVM
versions if you
wish but your mileage may vary, only LLVM 22 is guaranteed to be OK.

### Building locally

To build locally pull the repo and ensure you have the following dependencies installed :-

- A gcc toolchain (including ccache). The version is not critical as long as c++23 is supported,
- A Bazel installation. Bazel is usually installed using the [bazelisk](https://github.com/bazelbuild/bazelisk) utility.
- An LLVM 22 installation with development libraries. LLVM 21 may be OK to use also.

Firstly check your installation, in the top level of the repo enter :

```
bazel info
```

This should give you output similar to this for your username :-

```
Starting local Bazel server (9.3.1) and connecting to it...
bazel-bin: /home/steve/.cache/bazel/_bazel_steve/8cbf2aec7a5d93f05b2c0fd2f9a7d513/execroot/_main/bazel-out/k8-dbg/bin
bazel-genfiles: /home/steve/.cache/bazel/_bazel_steve/8cbf2aec7a5d93f05b2c0fd2f9a7d513/execroot/_main/bazel-out/k8-dbg/bin
bazel-testlogs: /home/steve/.cache/bazel/_bazel_steve/8cbf2aec7a5d93f05b2c0fd2f9a7d513/execroot/_main/bazel-out/k8-dbg/testlogs
character-encoding: file.encoding = ISO-8859-1, defaultCharset = ISO-8859-1, sun.jnu.encoding = ISO-8859-1
command_log: /home/steve/.cache/bazel/_bazel_steve/8cbf2aec7a5d93f05b2c0fd2f9a7d513/command.log
committed-heap-size: 92MB
execution_root: /home/steve/.cache/bazel/_bazel_steve/8cbf2aec7a5d93f05b2c0fd2f9a7d513/execroot/_main
gc-count: 16
gc-time: 51ms
install_base: /home/steve/.cache/bazel/_bazel_steve/install/1fa1a41c88a552221379e9a42f282033
java-home: /home/steve/.cache/bazel/_bazel_steve/install/1fa1a41c88a552221379e9a42f282033/embedded_tools/jdk
java-runtime: OpenJDK Runtime Environment (build 25.0.2+10-LTS) by Azul Systems, Inc.
java-vm: OpenJDK 64-Bit Server VM (build 25.0.2+10-LTS, mixed mode) by Azul Systems, Inc.
local_resources: RAM=63830MB, CPU=20.0
max-heap-size: 16735MB
output_base: /home/steve/.cache/bazel/_bazel_steve/8cbf2aec7a5d93f05b2c0fd2f9a7d513
output_path: /home/steve/.cache/bazel/_bazel_steve/8cbf2aec7a5d93f05b2c0fd2f9a7d513/execroot/_main/bazel-out
package_path: %workspace%
release: release 9.3.1
repository_cache: /home/steve/.cache/bazel/_bazel_steve/cache/repos/v1
server_log: /home/steve/.cache/bazel/_bazel_steve/8cbf2aec7a5d93f05b2c0fd2f9a7d513/java.log.localhost.steve.log.java.20261029-105356.558040
server_pid: 558040
used-heap-size: 24MB
workspace: /home/steve/Development/c++/coral/builds/coral66-ng
```

This shows bazel information such as output paths, log paths etc.

To check all the bazel targets are present use :-

```
bazel query //... | wc -l
```

The result of which should be 48. To list each target name just remove the 'wc -l' command.

To build the compiler enter the following :-

```aiignore
bazel build //...
```

This performs a full build, an example of the output is :-

```
INFO: Analyzed 47 targets (84 packages loaded, 1193 targets configured).
INFO: Found 47 targets...
INFO: Elapsed time: 3.158s, Critical Path: 0.81s
INFO: 247 processes: 76 internal, 171 linux-sandbox.
INFO: Build completed successfully, 247 total actions
```

To run the unit test suite enter :-

```
bazel test //...
```

This command will perform a build if needed and run the unit test suite, an example of the output is :=

```
INFO: Analyzed 47 targets (0 packages loaded, 0 targets configured).
INFO: Found 38 targets and 9 test targets...
INFO: Elapsed time: 8.572s, Critical Path: 8.28s
INFO: 10 processes: 1 internal, 9 linux-sandbox.
INFO: Build completed successfully, 10 total actions
//test/backend:backend                                                   PASSED in 8.2s
//test/compliance:compliance                                             PASSED in 1.2s
//test/frontend/common:common_test                                       PASSED in 0.1s
//test/frontend/lexer:lexer                                              PASSED in 0.1s
//test/frontend/parser:parser                                            PASSED in 0.1s
//test/frontend/preprocessor:preprocessor                                PASSED in 0.1s
//test/frontend/semantics:semantics                                      PASSED in 0.2s
//test/main:main                                                         PASSED in 0.1s
//test/support:support                                                   PASSED in 0.1s

Executed 9 out of 9 tests: 9 tests pass.
```

Note that bazel caches the results of the tests so if you run this again without changing anything the cached test
results are shown,
i.e the annotation '(cached)' is added to the test results for a target, example :-

```
//test/backend:backend                                          (cached) PASSED in 8.2s
```

Test logs if needed van be found in the 'bazel-testlogs' directory from the info above.

You may also get a notification about a service or application crash, this is expected, one of the unit tests is
expected to crash.

When built the compiler is found in the 'bazel-bin' directory from the info output above. See below for installation.

If you wish to rebuild from a fresh start (bazel builds incrementally) then enter :-

```
bazel clean
```

### Building using a podman development environment

The top level script developer-podman.sh builds the compiler from your pulled repo but in a podman container
so you do not need to worry about dependencies. The script runs the build and tests placing the compiler
executable and logfiles in the podman-build directory.

This allows a developer to change repository content and rebuild without re pulling the repository every time.

To run the script enter :-

```
./developer-podman.sh
```

The script creates a Fedora 44 container image to perform the build. Progress of the build is
output to the terminal with detailed information in the logfiles in the podman-build directory.

Note that if you are on a selinux enabled platform such as Fedora you may get selinux alerts during
the build process, these will not allow the compiler binary to be generated correctly. To alleviate this
the selinux directory contains a commands.txt file, run these commands, follow the on screen
instructions and re run the developer-podman.sh script, this should fix these problems.

The end of the terminal output should show this for a successful build :-
```
=================================================
 Running Bazel tests and build inside Podman
=================================================
Creating required ccache directory for sandbox...
Bazel Information...
Running bazel build...
Running bazel tests...
Copying produced coral66 binary to podman-build...
Info, build and test phases completed successfully.
=================================================
 Build Completed successfully!
=================================================
The coral66 binary is available at: ./podman-build/coral66
The test logs are available at: ./podman-build/bazel_test.log
The build logs are available at: ./podman-build/bazel_build.log
```

### Building using a podman user environment
This performs the same function as the podman developer build except the repo is pulled into the container image and built from there. This allows a
more hermetic build but does limit builds to the repo contents. The outputs are in a clean_build_artifacts directory so as not to overwrite the developer podman build.
It is recommended the user-podman.sh script is copied into its own directory and run from there.

Note that the repo branch is defaulted to main but can be changed by editing the 'BRANCH' environment variable at the top of the script.

## Installation
Installation is a simple matter of copying the built coral66 binary to somewhere on your path, or add its location to your path. The install.sh script in
scripts will install the binary in your '$USER/bin' directory as this is on your path by default. 

Note that the helper scripts in the scripts directory do assume the coral66 binary is on your path.

As stated above the compiler does use gcc to perform linking operations and executable generation so a gcc toolchain will be needed in your runtime environment.