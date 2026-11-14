#!/bin/bash
set -e

# ==============================================================================
# Script to automate the build of coral66-ng in a Fedora 45 Podman container
# ==============================================================================

echo "================================================="
echo " Generating Containerfile for Fedora 45 builder"
echo "================================================="

cat << 'EOF' > ./podman-build/Containerfile.fedora45
# 1. Pull down the latest Fedora 45 image
FROM registry.fedoraproject.org/fedora:45

# 2. Install full SW development environment and LLVM dev packages
RUN dnf -y update && \
    dnf -y group install development-tools && \
    dnf -y install llvm-devel lld curl wget findutils gcc-c++ && \
    dnf -y clean all

# 3. Install Bazelisk as /usr/local/bin/bazel
RUN curl -L "https://github.com/bazelbuild/bazelisk/releases/latest/download/bazelisk-linux-amd64" -o /usr/local/bin/bazel && \
    chmod +x /usr/local/bin/bazel

WORKDIR /workspace
EOF

echo "================================================="
echo " Building Podman image (fedora45-coral66-builder)"
echo "================================================="
podman build -t fedora45-coral66-builder -f ./podman-build/Containerfile.fedora45 .

echo "================================================="
echo " Running Bazel tests and build inside Podman"
echo "================================================="
# We mount the current directory into /workspace.
# The :Z flag ensures SELinux context is correctly handled on Fedora/RHEL.
podman run --rm -v "$(pwd)":/workspace:Z fedora45-coral66-builder /bin/bash -c "
    cleanup_symlinks() {
        echo 'Removing temporary podman-* symlinks...'
        rm -f podman-bin podman-out podman-testlogs podman-workspace
    }
    trap cleanup_symlinks EXIT

    echo 'Creating required ccache directory for sandbox...'
    sudo mkdir -p /var/cache/ccache
    sudo chown \$(whoami):\$(whoami) /var/cache/ccache
    sudo chmod 1777 /var/cache/ccache

    echo 'Bazel Information...'
    # Bazel build and install info
    mkdir -p ./podman-build/logs
    rm -f ./podman-build/logs/*.log
    bazel info > ./podman-build/logs/bazel_info.log 2>&1 || { echo 'Bazel info failed, check ./podman-build/logs/bazel_info.log'; exit 1; }

    echo 'Running bazel build...'
    # Build everything and output log to host directory
    bazel build --symlink_prefix=podman- //... > ./podman-build/logs/bazel_build.log 2>&1 || { echo 'Bazel build failed, check ./podman-build/logs/bazel_build.log'; exit 1; }

    copy_test_logs() {
        echo 'Copying Catch2 test logs to podman-build/logs...'
        cp -f podman-testlogs/test/backend/backend/test.log ./podman-build/logs/backend_test.log 2>/dev/null || true
        cp -f podman-testlogs/test/compliance/compliance/test.log ./podman-build/logs/compliance_test.log 2>/dev/null || true
        cp -f podman-testlogs/test/frontend/common/common_test/test.log ./podman-build/logs/frontend_common_test.log 2>/dev/null || true
        cp -f podman-testlogs/test/frontend/lexer/lexer/test.log ./podman-build/logs/frontend_lexer_test.log 2>/dev/null || true
        cp -f podman-testlogs/test/frontend/parser/parser/test.log ./podman-build/logs/frontend_parser_test.log 2>/dev/null || true
        cp -f podman-testlogs/test/frontend/preprocessor/preprocessor/test.log ./podman-build/logs/frontend_preprocessor_test.log 2>/dev/null || true
        cp -f podman-testlogs/test/frontend/semantics/semantics/test.log ./podman-build/logs/frontend_semantics_test.log 2>/dev/null || true
        cp -f podman-testlogs/test/main/main/test.log ./podman-build/logs/main_test.log 2>/dev/null || true
        cp -f podman-testlogs/test/support/support/test.log ./podman-build/logs/support_test.log 2>/dev/null || true
    }

    echo 'Running bazel tests...'
    # Run tests and output log to host directory
    bazel test --symlink_prefix=podman- //... > ./podman-build/logs/bazel_test.log 2>&1 || { copy_test_logs; echo 'Bazel test failed, check logs in ./podman-build/logs/'; exit 1; }
    copy_test_logs
    
    echo 'Copying produced coral66 binary to podman-build...'
    cp podman-bin/bin/coral66 ./podman-build/coral66

    echo 'Copying produced tools binaries to podman-build/tools...'
    mkdir -p ./podman-build/tools
    cp podman-bin/tools/parser/astdump/bin/astdump ./podman-build/tools/astdump
    cp podman-bin/tools/preprocessor/ppdump/bin/ppdump ./podman-build/tools/ppdump
    cp podman-bin/tools/semantics/programdump/bin/programdump ./podman-build/tools/programdump
    
    echo 'Info, build and test phases completed successfully.'
"

echo "================================================="
echo " Build Completed successfully!"
echo "================================================="
echo "The coral66 binary is available at: ./podman-build/coral66"
echo "The tools binaries are available at: ./podman-build/tools/"
echo "The logs are available at: ./podman-build/logs/"
