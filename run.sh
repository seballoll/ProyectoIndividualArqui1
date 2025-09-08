#!/bin/bash
IMAGE_NAME="rvqemu"

# Detect container runtime (podman vs docker)
if command -v podman >/dev/null 2>&1 && [[ -z "$DOCKER_HOST" ]]; then
    CONTAINER_CMD="podman"
    echo "Detected Podman runtime"
else
    CONTAINER_CMD="docker"
    echo "Detected Docker runtime"
fi

# Get current user info
HOST_UID=$(id -u)
HOST_GID=$(id -g)
HOST_USER=$(id -un)

echo "Host user: $HOST_USER (UID: $HOST_UID, GID: $HOST_GID)"

# Build the Docker image if it doesn't exist
if [[ "$($CONTAINER_CMD images -q $IMAGE_NAME 2> /dev/null)" == "" ]]; then
    echo "Building container image '$IMAGE_NAME'..."
    $CONTAINER_CMD build --tag $IMAGE_NAME \
        --build-arg UID="$HOST_UID" \
        --build-arg GID="$HOST_GID" \
        --build-arg USERNAME="rvqemu-dev" \
        --build-arg CACHEBUST=$(date +%s) .
else
    echo "Container image '$IMAGE_NAME' already exists. Skipping build."
fi

# Set up volume mount and working directory
VOLUME_MOUNT="/home/rvqemu-dev/workspace"

# Run the container
echo "Starting container '$IMAGE_NAME'..."

if [[ "$CONTAINER_CMD" == "podman" ]]; then
    # Podman handles user namespaces better
    $CONTAINER_CMD run --rm -it --name rvqemu \
        -v "$(pwd):$VOLUME_MOUNT" \
        --userns=keep-id \
        --workdir $VOLUME_MOUNT \
        $IMAGE_NAME /bin/bash
else
    # Docker - run as the created user
    $CONTAINER_CMD run --rm -it --name rvqemu \
        -v "$(pwd):$VOLUME_MOUNT" \
        --user rvqemu-dev \
        --workdir $VOLUME_MOUNT \
        $IMAGE_NAME /bin/bash
fi
