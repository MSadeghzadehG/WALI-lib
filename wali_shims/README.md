# wali_shims - WASM Import Headers

This directory contains shim headers that replace standard library headers during WASM compilation. These headers declare functions as WASM imports rather than providing implementations.

## Currently Supported

| Library | Header | Functions | Status |
|---------|--------|-----------|--------|
| zlib | `zlib.h` | 90+ | Complete |

## How It Works

```
┌─────────────────────────────────────────────────────────────┐
│                     WASM Module                              │
│  #include <zlib.h>  // Uses wali_shims/zlib.h               │
│  compress(...)      // Calls wali_compress import           │
└──────────────────────────┬──────────────────────────────────┘
                           │ WASM import
                           ▼
┌─────────────────────────────────────────────────────────────┐
│                     WAMR Runtime                             │
│  lib_zlib.c         // Registered as "env" native functions │
│  zlib_compress_wrapper() → host libz.so                     │
└─────────────────────────────────────────────────────────────┘
```

### Standard Header (system)
```c
// /usr/include/zlib.h
int compress(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen);
// Links to libz.so at build time
```

### Shim Header (wali_shims)
```c
// wali_shims/zlib.h
__attribute__((import_module("env"), import_name("wali_compress")))
int compress(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen);
// Creates WASM import, resolved at runtime by WAMR
```

## Usage

When compiling for WALI, include this directory before system includes:

```bash
clang --target=wasm32-unknown-linux-muslwali \
      --sysroot=$WALI_SYSROOT \
      -I/path/to/wali_shims \
      -o app.wasm app.c
```

The resulting WASM binary will have imports like:
```wat
(import "env" "wali_compress" (func $compress (param i32 i32 i32 i32) (result i32)))
```

## zlib.h

Complete zlib support including:

- **Basic**: `compress`, `uncompress`, `compressBound`
- **Deflate**: `deflateInit`, `deflate`, `deflateEnd`, `deflateSetHeader`, etc.
- **Inflate**: `inflateInit`, `inflate`, `inflateEnd`, `inflateGetHeader`, etc.
- **Gzip I/O**: `gzopen`, `gzread`, `gzwrite`, `gzclose`, `gzseek`, etc.
- **Utilities**: `adler32`, `crc32`, `zlibVersion`, etc.

See `wasm-micro-runtime/core/iwasm/libraries/lib-zlib/README.md` for full API documentation.

## Adding New Libraries

1. **Create shim header** in `wali_shims/`:
   ```c
   // wali_shims/mylib.h
   __attribute__((import_module("env"), import_name("wali_myfunc")))
   int myfunc(int arg);
   ```

2. **Implement WAMR wrapper** in `wasm-micro-runtime/core/iwasm/libraries/lib-mylib/`:
   ```c
   #include "../lib_native_utils.h"
   #include <mylib.h>
   
   static int mylib_myfunc_wrapper(wasm_exec_env_t exec_env, int arg) {
       return myfunc(arg);
   }
   
   static NativeSymbol native_symbols[] = {
       NATIVE_FUNC(wali_myfunc, mylib_myfunc_wrapper, "(i)i"),
   };
   ```

3. **Add CMake integration** and link against host library

## Testing

```bash
cd tests/zlib_test
./compile.sh
../../iwasm test_zlib.wasm
```
