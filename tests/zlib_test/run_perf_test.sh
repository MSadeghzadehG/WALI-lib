#!/bin/bash
# run_perf_test.sh - Compile and run zlib performance comparison
#
# This script:
# 1. Compiles perf_zlib.c natively with gcc
# 2. Compiles perf_zlib.c for WASM using WALI toolchain
# 3. Runs both versions and displays results

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WALI_ROOT="${SCRIPT_DIR}/../.."

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

echo -e "${BLUE}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║         zlib Performance Comparison: Native vs WASM          ║${NC}"
echo -e "${BLUE}╚══════════════════════════════════════════════════════════════╝${NC}"
echo ""

# Check for required tools
check_tool() {
    if ! command -v "$1" &> /dev/null; then
        echo -e "${RED}Error: $1 is not installed${NC}"
        exit 1
    fi
}

check_tool gcc

# Paths
PERF_SRC="${SCRIPT_DIR}/perf_zlib.c"
NATIVE_BIN="${SCRIPT_DIR}/perf_zlib_native"
WASM_BIN="${SCRIPT_DIR}/perf_zlib.wasm"
IWASM="${WALI_ROOT}/build/wamr/iwasm/iwasm"
WALI_ENV="${WALI_ROOT}/.walienv"

# WALI toolchain
WALI_CC="${WALI_ROOT}/build/llvm/bin/clang"
WALI_SYSROOT="${WALI_ROOT}/build/sysroot"

# ============================================================================
# Step 1: Compile native version
# ============================================================================
echo -e "${YELLOW}[1/4] Compiling native version...${NC}"
gcc -O2 -o "${NATIVE_BIN}" "${PERF_SRC}" -lz
echo -e "${GREEN}  ✓ Native binary: ${NATIVE_BIN}${NC}"

# ============================================================================
# Step 2: Compile WASM version
# ============================================================================
echo -e "${YELLOW}[2/4] Compiling WASM version...${NC}"

if [ ! -f "${WALI_CC}" ]; then
    echo -e "${RED}  ✗ WALI compiler not found at: ${WALI_CC}${NC}"
    echo -e "${YELLOW}  Skipping WASM compilation...${NC}"
    SKIP_WASM=1
else
    # Use the same flags as compile.sh
    "${WALI_CC}" \
        --target=wasm32-unknown-linux-muslwali \
        --sysroot="${WALI_SYSROOT}" \
        -O2 \
        -pthread \
        -fwasm-exceptions \
        -Wno-implicit-function-declaration \
        -Wno-int-conversion \
        -mcpu=generic \
        -matomics \
        -mbulk-memory \
        -mexception-handling \
        -I"${WALI_ROOT}/cpython/wali_shims" \
        -L"${WALI_SYSROOT}/lib" \
        -Wl,--shared-memory \
        -Wl,--export-memory \
        -Wl,--max-memory=2147483648 \
        -o "${WASM_BIN}" \
        "${PERF_SRC}"
    echo -e "${GREEN}  ✓ WASM binary: ${WASM_BIN}${NC}"
    SKIP_WASM=0
fi

# ============================================================================
# Step 3: Run native version
# ============================================================================
echo ""
echo -e "${CYAN}════════════════════════════════════════════════════════════════${NC}"
echo -e "${CYAN}                    NATIVE EXECUTION                            ${NC}"
echo -e "${CYAN}════════════════════════════════════════════════════════════════${NC}"
echo ""

"${NATIVE_BIN}"
NATIVE_EXIT=$?

if [ $NATIVE_EXIT -ne 0 ]; then
    echo -e "${RED}Native execution failed with exit code: ${NATIVE_EXIT}${NC}"
fi

# ============================================================================
# Step 4: Run WASM version
# ============================================================================
if [ "${SKIP_WASM}" = "1" ]; then
    echo ""
    echo -e "${YELLOW}Skipping WASM execution (compilation failed)${NC}"
else
    echo ""
    echo -e "${CYAN}════════════════════════════════════════════════════════════════${NC}"
    echo -e "${CYAN}                    WASM EXECUTION (iwasm)                      ${NC}"
    echo -e "${CYAN}════════════════════════════════════════════════════════════════${NC}"
    echo ""

    if [ ! -f "${IWASM}" ]; then
        echo -e "${RED}  ✗ iwasm not found at: ${IWASM}${NC}"
        echo -e "${YELLOW}  Please build iwasm first:${NC}"
        echo -e "${YELLOW}    cd ${WALI_ROOT}/build/wamr/iwasm && ninja${NC}"
    else
        # Run with WALI env file, suppress link warnings
        "${IWASM}" \
            --env-file="${WALI_ENV}" \
            --stack-size=8388608 \
            --heap-size=134217728 \
            --dir=/ \
            "${WASM_BIN}" 2>&1 | grep -v "warning: failed to link"
        
        WASM_EXIT=${PIPESTATUS[0]}
        
        if [ $WASM_EXIT -ne 0 ]; then
            echo -e "${RED}WASM execution failed with exit code: ${WASM_EXIT}${NC}"
        fi
    fi
fi

echo ""
echo -e "${BLUE}════════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}                    COMPARISON COMPLETE                         ${NC}"
echo -e "${BLUE}════════════════════════════════════════════════════════════════${NC}"
