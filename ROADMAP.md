# wali-lib Roadmap

## Philosophy

**The Problem**: WASI tries to standardize everything, but it's impossible to encode all human-generated knowledge and code into a single standard. There are millions of libraries covering every domain imaginable—compression, cryptography, databases, GUI, machine learning, networking, and more. No standards committee can keep up with this pace of evolution.

**The Solution**: A pragmatic two-tier approach:

1. **wali-lib**: Host-side wrappers for critical infrastructure libraries that are:
   - Performance-critical
   - Security-sensitive  
   - Universally needed
   - Stable ABI
   - Impractical to rewrite

2. **Everything else**: Developers rewrite or port their specific needs in WASM-compatible ways (compile to WASM directly, use WASM-native alternatives, or rewrite in Rust/Zig targeting WASM).

```
┌─────────────────────────────────────────────────────────────┐
│              All Libraries in the World                      │
└─────────────────────────────────────────────────────────────┘
                            │
            ┌───────────────┴───────────────┐
            ▼                               ▼
    ┌───────────────┐               ┌───────────────────┐
    │  Critical     │               │  Application      │
    │  Infra (~20)  │               │  Libraries        │
    │               │               │  (millions)       │
    │  wali-lib     │               │                   │
    │  host-side    │               │  Rewrite/port     │
    │  wrappers     │               │  to pure WASM     │
    └───────────────┘               └───────────────────┘
```

## Why wali-lib?

| Aspect | wali-lib (host wrappers) | Pure WASM port |
|--------|--------------------------|----------------|
| Performance | Native speed | WASM overhead |
| Security | Battle-tested implementations | May introduce bugs |
| Effort | One-time wrapper work | Full port/rewrite |
| Portability | WAMR (+ adapters for others) | Any WASM runtime |
| Maintenance | Host lib updates automatically | Must track upstream |

## Library Selection Criteria

A library should be in wali-lib if it meets **3+ of these criteria**:

- [ ] **Performance-critical**: Heavy computation (compression, crypto, image processing)
- [ ] **Security-sensitive**: Cryptography, TLS—don't want WASM reimplementations
- [ ] **Universally needed**: Used by >50% of real-world applications
- [ ] **Stable ABI**: Mature, rarely changes struct layouts
- [ ] **Hard to port**: Uses SIMD, assembly, OS-specific optimizations
- [ ] **Large codebase**: Impractical to rewrite (>50k LOC)

---

## Library Tiers

### Tier 1: Critical Infrastructure (Must Have)

These libraries are foundational—almost every non-trivial application needs them.

| Library | Category | Status | Priority | Notes |
|---------|----------|--------|----------|-------|
| **zlib** | Compression | ✅ Done | - | 73 functions implemented |
| **OpenSSL** / **libcrypto** | Cryptography | ❌ Not started | P0 | Security-critical, don't reimplement |
| **SQLite** | Database | ❌ Not started | P0 | Single-file DB, universally used |
| **libpng** | Image | ❌ Not started | P1 | Depends on zlib |
| **libjpeg-turbo** | Image | ❌ Not started | P1 | SIMD-optimized |
| **libcurl** | Networking | ❌ Not started | P1 | HTTP client, depends on OpenSSL |
| **zstd** | Compression | ❌ Not started | P1 | Modern compression, Facebook |
| **bzip2** | Compression | ❌ Not started | P2 | Older but still used |
| **lzma/xz** | Compression | ❌ Not started | P2 | High compression ratio |

### Tier 2: Important Infrastructure (Should Have)

These are commonly needed but more domain-specific.

| Library | Category | Status | Priority | Notes |
|---------|----------|--------|----------|-------|
| **libwebp** | Image | ❌ Not started | P2 | Modern image format |
| **freetype** | Font | ❌ Not started | P2 | Font rendering |
| **libxml2** | Data | ❌ Not started | P2 | XML parsing |
| **expat** | Data | ❌ Not started | P3 | Lightweight XML |
| **libyaml** | Data | ❌ Not started | P3 | YAML parsing |
| **pcre2** | Text | ❌ Not started | P2 | Regex engine |
| **libffi** | FFI | ❌ Not started | P2 | Foreign function interface |
| **libuv** | Async I/O | ❌ Not started | P2 | Event loop (Node.js style) |
| **mbedtls** | Crypto | ❌ Not started | P2 | Lightweight TLS alternative |

### Tier 3: Nice to Have

| Library | Category | Status | Priority | Notes |
|---------|----------|--------|----------|-------|
| **libsodium** | Crypto | ❌ Not started | P3 | Modern crypto API |
| **libevent** | Async I/O | ❌ Not started | P3 | Event notification |
| **gmp** | Math | ❌ Not started | P3 | Arbitrary precision |
| **libarchive** | Archive | ❌ Not started | P3 | tar, zip, etc. |
| **libgit2** | VCS | ❌ Not started | P3 | Git operations |
| **leveldb** | Database | ❌ Not started | P3 | Key-value store |

---

## Implementation Roadmap

### Phase 1: Foundation (Current)
- [x] zlib - Complete compression support
- [ ] OpenSSL/libcrypto - Core cryptography
- [ ] SQLite - Embedded database

### Phase 2: Media & Network
- [ ] libpng - PNG images
- [ ] libjpeg-turbo - JPEG images  
- [ ] libcurl - HTTP client
- [ ] zstd - Modern compression

### Phase 3: Extended Support
- [ ] libwebp, freetype - Additional media
- [ ] libxml2, pcre2 - Text processing
- [ ] bzip2, lzma - Legacy compression

### Phase 4: Ecosystem
- [ ] libuv - Async I/O
- [ ] libffi - FFI support
- [ ] Community-requested libraries

---

## Implementation Guide

### Adding a New Library to wali-lib

1. **Analyze the library**
   - Identify all public API functions
   - Categorize: simple (pass-through) vs complex (needs handle tables)
   - Check for callbacks, complex structs, thread usage

2. **Create shim header** (`wali_shims/<lib>.h`)
   - Declare all functions with `__attribute__((import_module("env"), import_name("wali_...")))`
   - Match original header signatures exactly

3. **Implement wrappers** (`wasm-micro-runtime/core/iwasm/libraries/lib-<name>/`)
   - Create `lib_<name>.c` with native wrapper functions
   - Add handle tables for opaque types (streams, handles, contexts)
   - Implement struct sync for complex structures
   - Register native symbols (both original and `wali_` prefixed)

4. **Update build system**
   - Create `lib_<name>.cmake` with `find_package()` for the library
   - Add to WAMR build configuration

5. **Write tests** (`tests/<lib>_test/`)
   - Cover all major API functions
   - Test edge cases and error handling

6. **Document** 
   - Add README in the library directory
   - Update this roadmap

### Complexity Guide

| Pattern | Complexity | Example |
|---------|------------|---------|
| Pure functions | Easy | `crc32()`, `compress()` |
| Handle-based API | Medium | `gzopen()`, `deflateInit()` |
| Struct with pointers | Medium-Hard | `z_stream` |
| Callbacks | Hard | `inflateBack()` (skipped) |
| Thread-spawning | Very Hard | Avoid if possible |

---

## Non-Goals

Libraries that should **NOT** be in wali-lib:

- **GUI toolkits** (Qt, GTK) - Too complex, callback-heavy
- **Game engines** - Domain-specific, better as native
- **ML frameworks** (TensorFlow, PyTorch) - Massive, GPU-dependent
- **Language runtimes** (Python, Ruby) - Should run as WASM themselves
- **Application frameworks** - Too opinionated, rewrite instead

---

## Contributing

We welcome contributions! Priority areas:

1. **OpenSSL wrappers** - Most impactful next step
2. **SQLite wrappers** - Widely requested
3. **Tests for existing libraries** - Improve coverage
4. **Documentation** - Usage examples, guides

See the implementation guide above for how to add a new library.

---

## Status Legend

- ✅ Done - Fully implemented and tested
- 🚧 In Progress - Partially implemented
- ❌ Not started - On roadmap but no work done
- ⏸️ Blocked - Waiting on dependencies or decisions

**Priority Legend**:
- P0 = Critical, needed for basic functionality
- P1 = High, needed for common use cases
- P2 = Medium, useful but not urgent
- P3 = Low, nice to have
