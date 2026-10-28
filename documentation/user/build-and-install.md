
# Building and installing

## Building 
There are three ways to build the compiler, locally, using a developer podman script and using a user podman script.
The podman scripts obviously need podman installed on your platform, the other options each have their own dependencies, see below.
The project was originally built on a Fedora 44 platform. This comes with LLVM 22 as standard. You can use lower LLVM versions if you
wish but your mileage may vary, only LLVM 22 is guaranteed to be OK.

## Building locally
