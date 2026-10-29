
# Building and installing

## Building 
There are three ways to build the compiler, locally, using a developer podman script and using a user podman script.
The podman scripts obviously need podman installed on your platform, the other options each have their own dependencies, see below.
The project was originally built on a Fedora 44 platform. This comes with LLVM 22 as standard. You can use lower LLVM versions if you
wish but your mileage may vary, only LLVM 22 is guaranteed to be OK.

### Building locally
To build locally pull the repo and ensure you have the following dependencies installed :-

  - A gcc toolchain(including ccache). The version is not critical as long as c++23 is supported,
  - A Bazel installation. Bazel is usually installed using the [bazelisk](https://github.com/bazelbuild/bazelisk) utility.
  - An LLVM 22 installation with development libraries. LLVM 21 may be OK to use also.

Firstly check your installation, in the top level of the repo enter :
```
bazel info
```

This should give you output like this :-

