#!/bin/bash
set -e

# ==============================================================================
# Script to automate a clean-room build of coral66-ng in a Podman container.
# This pulls the repo into the container rather than relying on the local directory.
# ==============================================================================

REPO_URL="https://github.com/shamblett/coral66-ng.git"
BRANCH="${1:-main}" # Allow overriding the branch, default to main
CONTAINER_NAME="coral66-clean-builder-$(date +%s)"
IMAGE_NAME="fedora44-coral66-builder"

echo "================================================="
echo " Generating Containerfile for Fedora 44 builder"
echo "================================================="

cat << 'EOF' > Containerfile.fedora44
# 1. Pull down the latest Fedora 44 image
FROM registry.fedoraproject.org/fedora:44

# 2. Install full SW development environment, LLVM dev packages, and git
RUN dnf -y update && \
    dnf -y groupinstall "Development Tools" "C Development Tools and Libraries" && \
    dnf -y install llvm-devel clang lld curl wget findutils git && \
    dnf -y clean all

# 3. Install Bazelisk as /usr/local/bin/bazel
RUN curl -L "https://github.com/bazelbuild/bazelisk/releases/latest/download/bazelisk-linux-amd64" -o /usr/local/bin/bazel && \
    chmod +x /usr/local/bin/bazel

WORKDIR /workspace
EOF

echo "================================================="
echo " Building Podman image (${IMAGE_NAME})"
echo "================================================="
podman build -t "${IMAGE_NAME}" -f Containerfile.fedora44 .

echo "================================================="
echo " Starting clean build container..."
echo "================================================="
# Run the container in detached mode to execute commands inside it
podman run -d --name "$CONTAINER_NAME" "${IMAGE_NAME}" sleep infinity

# Ensure we cleanup the container on exit, whether successful or failed, and try to extract logs if they exist
cleanup() {
    echo 'Cleaning up and extracting logs if available...'
    # Create an output directory for clean build artifacts to avoid overwriting local dev state
    mkdir -p clean_build_artifacts

    # Attempt to copy logs if they exist
    podman cp "$CONTAINER_NAME":/workspace/coral66-ng/bazel_test.log ./clean_build_artifacts/bazel_test.log 2>/dev/null || true
    podman cp "$CONTAINER_NAME":/workspace/coral66-ng/bazel_build.log ./clean_build_artifacts/bazel_build.log 2>/dev/null || true
    podman cp "$CONTAINER_NAME":/workspace/coral66-ng/bazel-bin/bin/coral66 ./clean_build_artifacts/coral66 2>/dev/null || true
    
    echo 'Removing container...'
    podman rm -f "$CONTAINER_NAME" >/dev/null
}
trap cleanup EXIT

echo "================================================="
echo " Cloning repository (${BRANCH}) inside the container..."
echo "================================================="
podman exec "$CONTAINER_NAME" git clone --branch "$BRANCH" "$REPO_URL" /workspace/coral66-ng

echo "================================================="
echo " Running Bazel tests and build inside Podman"
echo "================================================="
# Execute the build commands inside the container
podman exec "$CONTAINER_NAME" /bin/bash -c "
    cd /workspace/coral66-ng
    
    echo 'Setting compiler to clang/LLVM...'
    export CC=clang
    export CXX=clang++

    echo 'Running bazel tests...'
    bazel test //... --repo_env=CC=clang --repo_env=CXX=clang++ > bazel_test.log 2>&1 || {
        echo 'Bazel test failed, extracting log...';
        exit 1;
    }
    
    echo 'Running bazel build...'
    bazel build //... --repo_env=CC=clang --repo_env=CXX=clang++ > bazel_build.log 2>&1 || {
        echo 'Bazel build failed, extracting log...';
        exit 1;
    }
    
    echo 'Build and test phases completed successfully.'
"

echo "================================================="
echo " Clean Build Completed successfully!"
echo "================================================="
echo "Artifacts will be extracted to ./clean_build_artifacts/ during cleanup"
