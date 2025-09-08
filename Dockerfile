ARG UID=1000
ARG GID=1000
ARG USERNAME=rvqemu-dev

FROM ubuntu:22.04
ARG UID
ARG GID
ARG USERNAME

# Avoid interactive prompts during package installs
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && \
    apt-get install -y git build-essential python3 python3-pip python3-venv \
                       pkg-config libglib2.0-dev libpixman-1-dev \
                       ninja-build wget curl sudo && \
    apt-get clean

# Install RISC-V GNU toolchain (RV32 and RV64 support)
RUN apt-get update && \
    apt-get install -y gcc-riscv64-unknown-elf gdb-multiarch

# Handle user and group creation based on UID/GID
RUN set -eux; \
    if getent group "$GID" >/dev/null; then \
        echo "Group with GID $GID already exists"; \
        EXISTING_GROUP=$(getent group "$GID" | cut -d: -f1); \
    else \
        groupadd -g "$GID" "$USERNAME"; \
        EXISTING_GROUP="$USERNAME"; \
    fi; \
    if id -u "$UID" >/dev/null 2>&1; then \
        echo "User with UID $UID already exists"; \
        EXISTING_USER=$(id -nu "$UID"); \
        usermod -g "$EXISTING_GROUP" "$EXISTING_USER"; \
        if [ "$EXISTING_USER" != "$USERNAME" ]; then \
            usermod -l "$USERNAME" -d "/home/$USERNAME" -m "$EXISTING_USER"; \
        fi; \
    else \
        useradd -m -u "$UID" -g "$EXISTING_GROUP" -s /bin/bash "$USERNAME"; \
    fi; \
    echo "$USERNAME ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

USER ${USERNAME}
WORKDIR /home/${USERNAME}

# Friendly prompt and PATH to local installs
RUN echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc && \
    echo 'export PS1="[\u@\h \W]\\$ "' >> ~/.bashrc

# Clone QEMU source (kept for students to modify)
RUN git clone https://gitlab.com/qemu-project/qemu.git /home/${USERNAME}/qemu-src && \
    cd /home/${USERNAME}/qemu-src && \
    git checkout stable-9.0

# Build & install QEMU to user prefix (no root needed later)
RUN cd /home/${USERNAME}/qemu-src && \
    ./configure --target-list=riscv32-softmmu,riscv64-softmmu \
                --enable-debug --prefix=/home/${USERNAME}/.local && \
    make -j"$(nproc)" && make install

# Helper script: rebuild QEMU after students edit device files
RUN printf '%s\n' \
'#!/usr/bin/env bash' \
'set -euo pipefail' \
'cd /home/${USERNAME}/qemu-src' \
'./configure --target-list=riscv32-softmmu,riscv64-softmmu --enable-debug --prefix="$HOME/.local"' \
'make -j"$(nproc)"' \
'make install' \
> /home/${USERNAME}/rebuild-qemu.sh && chmod +x /home/${USERNAME}/rebuild-qemu.sh