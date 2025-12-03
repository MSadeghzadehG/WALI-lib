#!/bin/bash
# Compile test_zlib.c to WASM using WALI toolchain

set -e

WALI_ROOT_DIR=/WALI
WALI_BUILD_DIR=$WALI_ROOT_DIR/build
WALI_LLVM_DIR=$WALI_BUILD_DIR/llvm
WALI_SYSROOT_DIR=$WALI_BUILD_DIR/sysroot
WALI_LLVM_BIN_DIR=$WALI_LLVM_DIR/bin

CC=$WALI_LLVM_BIN_DIR/clang
LD=$WALI_LLVM_BIN_DIR/wasm-ld

# Use the wali_shims zlib header
ZLIB_SHIM_DIR=$WALI_ROOT_DIR/cpython/wali_shims

CFLAGS="--target=wasm32-unknown-linux-muslwali \
    --sysroot=${WALI_SYSROOT_DIR} \
    -O2 \
    -pthread \
    -I${ZLIB_SHIM_DIR} \
    -Wno-implicit-function-declaration \
    -mcpu=generic \
    -matomics \
    -mbulk-memory"

LDFLAGS="-L${WALI_SYSROOT_DIR}/lib \
    -Wl,--shared-memory \
    -Wl,--export-memory \
    -Wl,--max-memory=2147483648 \
    -Wl,--allow-undefined"

echo "Compiling test_zlib.c..."
$CC $CFLAGS -c test_zlib.c -o test_zlib.o

echo "Linking test_zlib.wasm..."
$CC $CFLAGS $LDFLAGS test_zlib.o -o test_zlib.wasm

echo "Done! Created test_zlib.wasm"
ls -la test_zlib.wasm

echo ""
echo "Checking imports..."
wasm-objdump -x test_zlib.wasm 2>/dev/null | grep -E "Import\[" | head -20 || \
    $WALI_LLVM_BIN_DIR/llvm-nm test_zlib.wasm 2>/dev/null | grep " U " | head -20
