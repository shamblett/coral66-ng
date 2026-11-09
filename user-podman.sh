#!/bin/bash
set -e

# ==============================================================================
# Script to automate a clean-room build of coral66-ng in a Podman container.
# This pulls the repo into the container rather than relying on the local directory.
# ==============================================================================

REPO_URL="https://github.com/shamblett/coral66-ng.git"
BRANCH="${1:-main}" # Allow overriding the branch, default to main
CONTAINER_NAME="coral66-clean-builder-$(date +%s)"
IMAGE_NAME="fedora45-coral66-builder"

echo "================================================="
echo " Generating Containerfile for Fedora 45 builder"
echo "================================================="

# Create an output directory for clean build artifacts to avoid overwriting local dev state
mkdir -p user_build_artifacts

cat << 'EOF' > ./user_build_artifacts/Containerfile.fedora45
# 1. Pull down the latest Fedora 45 image
FROM registry.fedoraproject.org/fedora:45

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
echo " Building Podman image (${IMAGE_NAME})"
echo "================================================="
podman build -t "${IMAGE_NAME}" -f ./user_build_artifacts/Containerfile.fedora45 .

echo "================================================="
echo " Starting clean build container..."
echo "================================================="
# Run the container in detached mode to execute commands inside it
podman run -d --name "$CONTAINER_NAME" "${IMAGE_NAME}" sleep infinity

# Ensure we cleanup the container on exit, whether successful or failed, and try to extract logs if they exist
cleanup() {
    echo 'Cleaning up and extracting logs if available...'

    # Attempt to copy logs if they exist
    podman cp "$CONTAINER_NAME":/workspace/coral66-ng/bazel_info.log ./user_build_artifacts/bazel_info.log 2>/dev/null || true
    podman cp "$CONTAINER_NAME":/workspace/coral66-ng/bazel_test.log ./user_build_artifacts/bazel_test.log 2>/dev/null || true
    podman cp "$CONTAINER_NAME":/workspace/coral66-ng/bazel_build.log ./user_build_artifacts/bazel_build.log 2>/dev/null || true
    podman cp "$CONTAINER_NAME":/workspace/coral66-ng/bazel-bin/bin/coral66 ./user_build_artifacts/coral66 2>/dev/null || true
    
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

    echo 'Creating required ccache directory for sandbox...'
    sudo mkdir -p /var/cache/ccache
    sudo chown \$(whoami):\$(whoami) /var/cache/ccache
    sudo chmod 1777 /var/cache/ccache

    echo 'Bazel Information...'
    # Bazel build and install info
    bazel info > bazel_info.log 2>&1 || { echo 'Bazel info failed, check bazel_info.log'; exit 1; }

    echo 'Running bazel build...'
    # Build everything and output log to host directory
    bazel build //... > bazel_build.log 2>&1 || { echo 'Bazel build failed, check bazel_build.log'; exit 1; }

    echo 'Running bazel tests...'
    # Run tests and output log to host directory
    bazel test //... > bazel_test.log 2>&1 || { echo 'Bazel test failed, check bazel_test.log'; exit 1; }
    
    echo 'Info, build and test phases completed successfully.'
"

echo "================================================="
echo " Clean Build Completed successfully!"
echo "================================================="
echo "Artifacts will be extracted to ./user_build_artifacts/ during cleanup"
