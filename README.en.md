# wali-lib

**Native Library Support for WebAssembly via WALI**

A fork of [WALI (WebAssembly Linux Interface)](https://github.com/ArtlexArtem/wali) focused on providing native library bindings for WebAssembly applications.

## What is wali-lib?

wali-lib extends WALI to support native libraries (starting with zlib) in WebAssembly applications. It provides:

1. **Shim Headers** - Drop-in replacement headers that declare functions as WASM imports
2. **Native Wrappers** - WAMR host functions that call the actual native libraries
3. **Handle Tables** - Bridge between WASM's 32-bit pointers and native 64-bit pointers

## Currently Supported Libraries

| Library | Functions | Status |
|---------|-----------|--------|
| **zlib** | 90+ (compress, deflate, inflate, gzip file I/O) | Complete |

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                    WASM Application                             │
│                                                                 │
│  #include <zlib.h>     // Uses wali_shims/zlib.h                │
│  compress(dst, &len, src, srclen);                              │
│  gzFile f = gzopen("data.gz", "wb");                            │
└───────────────────────────┬─────────────────────────────────────┘
                            │ WASM imports
                            │ (import "env" "wali_compress" ...)
                            ▼
┌─────────────────────────────────────────────────────────────────┐
│                    WAMR Runtime (iwasm)                         │
│                                                                 │
│  lib-zlib/lib_zlib.c:                                           │
│    - Handle tables for z_stream, gzFile, gz_header              │
│    - Wrapper functions that translate WASM↔Native               │
│    - Links against host libz.so                                 │
└───────────────────────────┬─────────────────────────────────────┘
                            │ native calls
                            ▼
┌─────────────────────────────────────────────────────────────────┐
│                    Host System                                  │
│                    libz.so (native zlib)                        │
└─────────────────────────────────────────────────────────────────┘
```

## Quick Start

### 1. Build the Runtime

```bash
# Install dependencies
sudo ./apt-install-deps.sh

# Initialize submodules
git submodule update --init wasm-micro-runtime wali-musl

# Build iwasm with zlib support
make iwasm
```

### 2. Build the Toolchain (to compile WASM apps)

```bash
git submodule update --init --depth=1 llvm-project
make wali-compiler
make libc
```

### 3. Compile a zlib Application

```c
// myapp.c
#include <stdio.h>
#include <string.h>
#include <zlib.h>

int main() {
    const char *data = "Hello, wali-lib!";
    uLong src_len = strlen(data);
    uLong dst_len = compressBound(src_len);
    
    Bytef compressed[256];
    int ret = compress(compressed, &dst_len, (const Bytef *)data, src_len);
    
    if (ret == Z_OK) {
        printf("Compressed %lu bytes to %lu bytes\n", src_len, dst_len);
    }
    
    // Gzip file I/O
    gzFile f = gzopen("output.gz", "wb");
    gzwrite(f, data, src_len);
    gzclose(f);
    
    return 0;
}
```

Compile:
```bash
cd examples
./compile-wali-standalone.sh -I../wali_shims -o myapp.wasm myapp.c
../iwasm myapp.wasm
```

## Project Structure

```
wali-lib/
├── wali_shims/
│   └── zlib.h                 # WASM shim header (declares imports)
├── wasm-micro-runtime/
│   └── core/iwasm/libraries/
│       └── lib-zlib/
│           ├── lib_zlib.c     # Native wrappers (~1700 lines)
│           ├── lib_zlib.h     # API header
│           ├── lib_zlib.cmake # Build integration
│           └── README.md      # Detailed documentation
├── wali-musl/                 # Modified musl libc for WASM
├── tests/
│   └── zlib_test/
│       ├── test_zlib.c        # Core API tests
│       ├── test_gzip.c        # Gzip file I/O tests
│       └── compile.sh         # Build script
└── examples/
    └── compile-wali-standalone.sh
```

## zlib API Coverage

### Basic Compression
- `compress`, `compress2`, `compressBound`
- `uncompress`, `uncompress2`

### Deflate Stream
- `deflateInit`, `deflateInit2`, `deflate`, `deflateEnd`
- `deflateSetDictionary`, `deflateGetDictionary`
- `deflateParams`, `deflateTune`, `deflateBound`
- `deflatePending`, `deflatePrime`, `deflateSetHeader`
- `deflateCopy`, `deflateReset`, `deflateResetKeep`

### Inflate Stream
- `inflateInit`, `inflateInit2`, `inflate`, `inflateEnd`
- `inflateSetDictionary`, `inflateGetDictionary`
- `inflateSync`, `inflateMark`, `inflateGetHeader`
- `inflateCopy`, `inflateReset`, `inflateReset2`, `inflateResetKeep`
- `inflatePrime`, `inflateSyncPoint`, `inflateValidate`

### Gzip File I/O (27 functions)
- `gzopen`, `gzdopen`, `gzclose`, `gzclose_r`, `gzclose_w`
- `gzread`, `gzwrite`, `gzfread`, `gzfwrite`
- `gzgetc`, `gzputc`, `gzungetc`, `gzgets`, `gzputs`
- `gzseek`, `gztell`, `gzoffset`, `gzrewind`, `gzeof`
- `gzflush`, `gzbuffer`, `gzsetparams`, `gzdirect`
- `gzerror`, `gzclearerr`

### Utilities
- `adler32`, `adler32_z`, `adler32_combine`
- `crc32`, `crc32_z`, `crc32_combine`, `crc32_combine_gen`, `crc32_combine_op`
- `zlibVersion`, `zlibCompileFlags`, `zError`

## How It Works

### 1. Shim Header (`wali_shims/zlib.h`)

Replaces the system zlib.h during compilation:

```c
// Declares function as WASM import
__attribute__((import_module("env"), import_name("wali_compress")))
int compress(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen);
```

When compiled, this creates a WASM import:
```wat
(import "env" "wali_compress" (func $compress ...))
```

### 2. Native Wrapper (`lib_zlib.c`)

Registered with WAMR to handle imports:

```c
static int zlib_compress_wrapper(wasm_exec_env_t exec_env,
                                  uint32_t dest_wasm, uint32_t destLen_wasm,
                                  uint32_t source_wasm, uint32_t sourceLen) {
    // Translate WASM pointers to native
    Bytef *dest = WASM_PTR(exec_env, dest_wasm);
    uLongf *destLen = WASM_PTR(exec_env, destLen_wasm);
    const Bytef *source = WASM_PTR(exec_env, source_wasm);
    
    // Call real zlib
    return compress(dest, destLen, source, sourceLen);
}

// Register with WAMR
static NativeSymbol native_symbols_zlib[] = {
    NATIVE_FUNC(wali_compress, zlib_compress_wrapper, "(iiiI)i"),
    // ...
};
```

### 3. Handle Tables

For opaque types like `z_stream` and `gzFile`:

```c
// WASM can't hold native pointers (64-bit on host)
// So we use a handle table:

static z_stream *zstream_table[256];  // Native pointers
// WASM gets handle (index): 1, 2, 3, ...

gzFile gzopen(path, mode) {
    gzFile native = host_gzopen(path, mode);
    uint32_t handle = alloc_handle(native);  // Store in table
    return handle;  // Return 32-bit handle to WASM
}
```

## Running Tests

```bash
cd tests/zlib_test
./compile.sh
../../iwasm test_zlib.wasm   # Core API tests
../../iwasm test_gzip.wasm   # Gzip file I/O tests
```

## Adding New Libraries

To add support for another library (e.g., libpng):

1. **Create shim header**: `wali_shims/png.h`
   - Declare all functions with `__attribute__((import_module("env"), import_name("wali_...")))`

2. **Create native wrapper**: `wasm-micro-runtime/core/iwasm/libraries/lib-png/`
   - Implement wrapper functions
   - Create handle tables for opaque types
   - Register native symbols

3. **Update CMake**: Add to build system with `WAMR_BUILD_LIB_PNG=1`

4. **Add tests**: `tests/png_test/`

## Roadmap

See [ROADMAP.en.md](ROADMAP.en.md) for full details. | [Persian](ROADMAP.md)

### Planned Libraries

| Priority | Library | Category | Status |
|----------|---------|----------|--------|
| ✅ | zlib | Compression | Done |
| P0 | cJSON/jansson | JSON | Planned |
| P0 | hiredis | Redis/Cache | Planned |
| P0 | OpenSSL | Cryptography | Planned |
| P0 | SQLite | Database | Planned |
| P1 | libcurl | HTTP | Planned |
| P1 | libpq | PostgreSQL | Planned |
| P1 | zstd/brotli | Compression | Planned |
| P1 | libjwt | Authentication | Planned |
| P2 | libpng/libjpeg | Image | Planned |

### Implementation Phases

1. **Phase 1 (Current)**: zlib ✅, cJSON, hiredis, OpenSSL, SQLite
2. **Phase 2**: libcurl, libpq, zstd, brotli, libjwt
3. **Phase 3**: libpng, libjpeg, libxml2, pcre2
4. **Phase 4**: libsodium, mongo-c-driver, libuv, community libraries

## Differences from WASI

| Aspect | WASI | wali-lib |
|--------|------|----------|
| **Philosophy** | New portable API | Linux ABI + native libs |
| **File access** | Capability-based | Direct (like native) |
| **Libraries** | Must reimplement | Use native host libs |
| **Sandboxing** | Built-in | Trust-based |

## Credits

- Based on [WALI](https://github.com/ArtlexArtem/wali) - WebAssembly Linux Interface
- Uses [WAMR](https://github.com/bytecodealliance/wasm-micro-runtime) - WebAssembly Micro Runtime
- Uses [musl](https://musl.libc.org/) - Lightweight libc

## License

See [LICENSE](LICENSE) for details.
