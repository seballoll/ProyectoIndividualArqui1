#!/usr/bin/bash
# Build script for assembly-only example
echo "Building assembly-only example..."

riscv64-unknown-elf-gcc \
    -march=rv32im \
    -mabi=ilp32 \
    -nostdlib \
    -ffreestanding \
    -g \
    test.s \
    -T linker.ld \
    -o test.elf

if [ $? -eq 0 ]; then
    echo "Build successful: test.elf created"
else
    echo "Build failed"
    exit 1
fi