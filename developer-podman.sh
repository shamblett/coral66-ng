#!/bin/bash
set -e

# ==============================================================================
# Script to automate the build of coral66-ng in a Fedora 44 Podman container
# ==============================================================================

echo "================================================="
echo " Generating Containerfile for Fedora 44 builder"
echo "================================================="

cat << 'EOF' > ./podman-build/Containerfile.fedora44
# 1. Pull down the latest Fedora 44 image
FROM registry.fedoraproject.org/fedora:44

# 2. Install full SW development environment and LLVM dev packages
RUN dnf -y update && \
    dnf -y group install development-tools && \
    dnf -y install llvm-devel curl wget findutils gcc-c++ && \
    dnf -y clean all

# 3. Install Bazelisk as /usr/local/bin/bazel
RUN curl -L "https://github.com/bazelbuild/bazelisk/releases/latest/download/bazelisk-linux-amd64" -o /usr/local/bin/bazel && \
    chmod +x /usr/local/bin/bazel

WORKDIR /workspace
EOF

echo "================================================="
echo " Building Podman image (fedora44-coral66-builder)"
echo "================================================="
podman build -t fedora44-coral66-builder -f ./podman-build/Containerfile.fedora44 .

echo "================================================="
echo " Running Bazel tests and build inside Podman"
echo "================================================="
# We mount the current directory into /workspace.
# The :Z flag ensures SELinux context is correctly handled on Fedora/RHEL.
podman run --rm -v "$(pwd)":/workspace:Z fedora44-coral66-builder /bin/bash -c "

    echo 'Creating required ccache directory for sandbox...'
    sudo mkdir -p /var/cache/ccache
    sudo chown \$(whoami):\$(whoami) /var/cache/ccache
    sudo chmod 1777 /var/cache/ccache

    echo 'Bazel Information...'
    # Bazel build and install info
    bazel info > ./podman-build/bazel_info.log 2>&1 || { echo 'Bazel info failed, check bazel_info.log'; exit 1; }

    echo 'Running bazel build...'
    # Build everything and output log to host directory
    bazel build --symlink_prefix=podman- //... > ./podman-build/bazel_build.log 2>&1 || { echo 'Bazel build failed, check bazel_build.log'; exit 1; }

    echo 'Running bazel tests...'
    # Run tests and output log to host directory
    bazel test --symlink_prefix=podman- //... > ./podman-build/bazel_test.log 2>&1 || { echo 'Bazel test failed, check bazel_test.log'; exit 1; }
    
    echo 'Copying produced coral66 binary to podman-build...'
    cp bazel-bin/bin/coral66 ./podman-build/coral66
    
    echo 'Info, build and test phases completed successfully.'
"

echo "================================================="
echo " Build Completed successfully!"
echo "================================================="
echo "The coral66 binary is available at: ./podman-build/coral66"
echo "The test logs are available at: ./podman-build/bazel_test.log"
echo "The build logs are available at: ./podman-build/bazel_build.log"
