#!/bin/bash
set -e # Exit on error

# --- CONFIGURATION ---
# Point this to your WALI installation folder
WALI_HOME=/WALI
WALI_CLANG="$WALI_HOME/build/llvm/bin/clang"
WAMRC="$WALI_HOME/wamrc"
IWASM="$WALI_HOME/iwasm"

# --- STEP 1: COMPILE C -> WASM (Using WALI Toolchain) ---
echo "Step 1: Compiling C to WALI-Wasm..."

# We link against 'wali' lib (-lwali)
# We set the target to wasm32-wasi
$WALI_CLANG demo.c -o demo.wasm \
    -target wasm32-wasi \
    -O3 \
    -lwali \
    -Wl,--export=malloc \
    -Wl,--export=free

echo "  -> Generated: demo.wasm"

# --- STEP 2: AOT COMPILATION (Wasm -> Native Code) ---
echo "Step 2: Transpiling Wasm to Native Machine Code (AOT)..."

# This converts the Wasm bytecode into x86_64 assembly
$WAMRC -o demo.aot demo.wasm

echo "  -> Generated: demo.aot (Native Binary Format)"

# --- STEP 3: RUNNING ---
echo "Step 3: Running Native AOT..."
echo "---------------------------------------------------"

# We map the current directory (.) so the code can write the file
$IWASM --dir=. demo.aot

echo "---------------------------------------------------"
echo "Check for 'wali_output.txt' in this folder!"

