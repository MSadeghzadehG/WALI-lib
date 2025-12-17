# wali-lib

**پشتیبانی از کتابخانه‌های نیتیو برای WebAssembly از طریق WALI**

یک فورک از [WALI (رابط لینوکس وب‌اسمبلی)](https://github.com/arjunr2/WALI) با تمرکز بر ارائه اتصالات کتابخانه‌های نیتیو برای برنامه‌های WebAssembly.

> [English Version](README.en.md)

## انگیزه

**مشکل: WASI نیاز به کامپایل مجدد همه چیز دارد**

WASI از لینک پویا (dynamic linking) پشتیبانی نمی‌کند، یعنی هر کتابخانه باید بازنویسی یا مجدداً به WASM سازگار با WASI کامپایل شود. برای اجرای یک برنامه ساده C که از zlib و libpq استفاده می‌کند، باید هر دو کتابخانه (و تمام وابستگی‌هایشان) را به WASM کامپایل کنید — که برای اکثر برنامه‌های واقعی عملی نیست.

این توسعه‌دهندگان را مجبور می‌کند که کل میراث نرم‌افزاری را فقط برای اجرای برنامه‌هایشان در WASM مجدداً کامپایل کنند.

**راه‌حل: استفاده مجدد از زیرساخت لینوکس**

لینوکس دهه‌ها زیرساخت آزموده شده دارد: **syscallها** و **کتابخانه‌ها**.

- [WALI](https://github.com/arjunr2/WALI) syscallهای لینوکس را برای WASM مجازی‌سازی می‌کند
- **wali-lib** این را گسترش می‌دهد تا کتابخانه‌های اشتراکی لینوکس (فایل‌های .so) را مجازی‌سازی کند

**هدف ما: توسعه طبیعی WASM**

با wali-lib، توسعه‌دهندگان می‌توانند:
- از کتابخانه‌های نیتیو همان‌طور که هستند استفاده کنند (zlib، OpenSSL، libpq و غیره) — بدون نیاز به کامپایل مجدد
- فقط کد برنامه خودشان را به WASM کامپایل کنند
- کتابخانه‌های شخص ثالث می‌توانند بدون تغییر نحوه استفاده از کتابخانه‌های نیتیو کامپایل مجدد شوند (همان `#include <zlib.h>`، همان API)

ما روی ۲۰-۳۰ کتابخانه زیرساختی حیاتی تمرکز می‌کنیم که ۸۰٪+ بارهای کاری واقعی را پوشش می‌دهند — «libc توسعه اپلیکیشن»: فشرده‌سازی، رمزنگاری، پایگاه داده، شبکه و تجزیه داده.

## wali-lib چیست؟

wali-lib قابلیت WALI را برای پشتیبانی از کتابخانه‌های نیتیو (با شروع از zlib) در برنامه‌های WebAssembly گسترش می‌دهد. این پروژه شامل موارد زیر است:

1. **هدرهای Shim** - هدرهای جایگزین که توابع را به عنوان importهای WASM تعریف می‌کنند
2. **Wrapperهای نیتیو** - توابع میزبان WAMR که کتابخانه‌های نیتیو واقعی را فراخوانی می‌کنند
3. **جداول Handle** - پل ارتباطی بین اشاره‌گرهای ۳۲ بیتی WASM و اشاره‌گرهای ۶۴ بیتی نیتیو

## کتابخانه‌های پشتیبانی‌شده

| کتابخانه | توابع | وضعیت |
|----------|-------|--------|
| **zlib** | بیش از ۹۰ تابع (compress، deflate، inflate، gzip file I/O) | کامل |

## معماری

</p>
<div dir="ltr">

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

</div>

## شروع سریع

### ۱. ساخت Runtime

<div dir="ltr">

```bash
# نصب وابستگی‌ها
sudo ./apt-install-deps.sh

# مقداردهی اولیه submoduleها
git submodule update --init wasm-micro-runtime wali-musl

# ساخت iwasm با پشتیبانی zlib
make iwasm
```

</div>

### ۲. ساخت Toolchain (برای کامپایل برنامه‌های WASM)

<div dir="ltr">

```bash
git submodule update --init --depth=1 llvm-project
make wali-compiler
make libc
```

</div>

### ۳. کامپایل یک برنامه zlib

<div dir="ltr">

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

</div>

کامپایل:

<div dir="ltr">

```bash
cd examples
./compile-wali-standalone.sh -I../wali_shims -o myapp.wasm myapp.c
../iwasm myapp.wasm
```

</div>

## ساختار پروژه

<div dir="ltr">

```
wali-lib/
├── wali_shims/
│   └── zlib.h                 # هدر shim برای WASM (تعریف importها)
├── wasm-micro-runtime/
│   └── core/iwasm/libraries/
│       └── lib-zlib/
│           ├── lib_zlib.c     # Wrapperهای نیتیو (~۱۷۰۰ خط)
│           ├── lib_zlib.h     # هدر API
│           ├── lib_zlib.cmake # یکپارچه‌سازی با سیستم ساخت
│           └── README.md      # مستندات جزئی
├── wali-musl/                 # کتابخانه musl libc اصلاح‌شده برای WASM
├── tests/
│   └── zlib_test/
│       ├── test_zlib.c        # تست‌های API اصلی
│       ├── test_gzip.c        # تست‌های Gzip file I/O
│       └── compile.sh         # اسکریپت ساخت
└── examples/
    └── compile-wali-standalone.sh
```

</div>

## پوشش API کتابخانه zlib

### فشرده‌سازی پایه
- `compress`، `compress2`، `compressBound`
- `uncompress`، `uncompress2`

### جریان Deflate
- `deflateInit`، `deflateInit2`، `deflate`، `deflateEnd`
- `deflateSetDictionary`، `deflateGetDictionary`
- `deflateParams`، `deflateTune`، `deflateBound`
- `deflatePending`، `deflatePrime`، `deflateSetHeader`
- `deflateCopy`، `deflateReset`، `deflateResetKeep`

### جریان Inflate
- `inflateInit`، `inflateInit2`، `inflate`، `inflateEnd`
- `inflateSetDictionary`، `inflateGetDictionary`
- `inflateSync`، `inflateMark`، `inflateGetHeader`
- `inflateCopy`، `inflateReset`، `inflateReset2`، `inflateResetKeep`
- `inflatePrime`، `inflateSyncPoint`، `inflateValidate`

### عملیات فایل Gzip (۲۷ تابع)
- `gzopen`، `gzdopen`، `gzclose`، `gzclose_r`، `gzclose_w`
- `gzread`، `gzwrite`، `gzfread`، `gzfwrite`
- `gzgetc`، `gzputc`، `gzungetc`، `gzgets`، `gzputs`
- `gzseek`، `gztell`، `gzoffset`، `gzrewind`، `gzeof`
- `gzflush`، `gzbuffer`، `gzsetparams`، `gzdirect`
- `gzerror`، `gzclearerr`

### توابع کمکی
- `adler32`، `adler32_z`، `adler32_combine`
- `crc32`، `crc32_z`، `crc32_combine`، `crc32_combine_gen`، `crc32_combine_op`
- `zlibVersion`، `zlibCompileFlags`، `zError`

## نحوه کار

### ۱. هدر Shim (`wali_shims/zlib.h`)

این هدر جایگزین zlib.h سیستم در زمان کامپایل می‌شود:

<div dir="ltr">

```c
// تعریف تابع به عنوان import در WASM
__attribute__((import_module("env"), import_name("wali_compress")))
int compress(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen);
```

</div>

پس از کامپایل، این کد یک import در WASM ایجاد می‌کند:

<div dir="ltr">

```wat
(import "env" "wali_compress" (func $compress ...))
```

</div>

### ۲. Wrapper نیتیو (`lib_zlib.c`)

این توابع در WAMR ثبت می‌شوند تا importها را مدیریت کنند:

<div dir="ltr">

```c
static int zlib_compress_wrapper(wasm_exec_env_t exec_env,
                                  uint32_t dest_wasm, uint32_t destLen_wasm,
                                  uint32_t source_wasm, uint32_t sourceLen) {
    // تبدیل اشاره‌گرهای WASM به نیتیو
    Bytef *dest = WASM_PTR(exec_env, dest_wasm);
    uLongf *destLen = WASM_PTR(exec_env, destLen_wasm);
    const Bytef *source = WASM_PTR(exec_env, source_wasm);
    
    // فراخوانی zlib واقعی
    return compress(dest, destLen, source, sourceLen);
}

// ثبت در WAMR
static NativeSymbol native_symbols_zlib[] = {
    NATIVE_FUNC(wali_compress, zlib_compress_wrapper, "(iiiI)i"),
    // ...
};
```

</div>

### ۳. جداول Handle

برای انواع opaque مانند `z_stream` و `gzFile`:

<div dir="ltr">

```c
// WASM نمی‌تواند اشاره‌گرهای نیتیو (۶۴ بیتی در میزبان) را نگه دارد
// بنابراین از جدول handle استفاده می‌کنیم:

static z_stream *zstream_table[256];  // اشاره‌گرهای نیتیو
// WASM یک handle (اندیس) دریافت می‌کند: ۱، ۲، ۳، ...

gzFile gzopen(path, mode) {
    gzFile native = host_gzopen(path, mode);
    uint32_t handle = alloc_handle(native);  // ذخیره در جدول
    return handle;  // بازگرداندن handle ۳۲ بیتی به WASM
}
```

</div>

## اجرای تست‌ها

<div dir="ltr">

```bash
cd tests/zlib_test
./compile.sh
../../iwasm test_zlib.wasm   # تست‌های API اصلی
../../iwasm test_gzip.wasm   # تست‌های Gzip file I/O
```

</div>

## افزودن کتابخانه‌های جدید

برای افزودن پشتیبانی از یک کتابخانه جدید (مثلاً libpng):

1. **ایجاد هدر shim**: `wali_shims/png.h`
   - تمام توابع را با `__attribute__((import_module("env"), import_name("wali_...")))` تعریف کنید

2. **ایجاد wrapper نیتیو**: `wasm-micro-runtime/core/iwasm/libraries/lib-png/`
   - توابع wrapper را پیاده‌سازی کنید
   - جداول handle برای انواع opaque ایجاد کنید
   - symbolهای نیتیو را ثبت کنید

3. **به‌روزرسانی CMake**: اضافه کردن به سیستم ساخت با `WAMR_BUILD_LIB_PNG=1`

4. **افزودن تست‌ها**: `tests/png_test/`

## نقشه راه

برای جزئیات کامل به [ROADMAP.md](ROADMAP.md) مراجعه کنید. | [English](ROADMAP.en.md)

### کتابخانه‌های برنامه‌ریزی شده

<div dir="ltr">

| اولویت | کتابخانه | دسته | وضعیت |
|--------|----------|------|--------|
| ✅ | zlib | فشرده‌سازی | انجام شده |
| P0 | cJSON/jansson | JSON | برنامه‌ریزی شده |
| P0 | hiredis | Redis/کش | برنامه‌ریزی شده |
| P0 | OpenSSL | رمزنگاری | برنامه‌ریزی شده |
| P0 | SQLite | پایگاه داده | برنامه‌ریزی شده |
| P1 | libcurl | HTTP | برنامه‌ریزی شده |
| P1 | libpq | PostgreSQL | برنامه‌ریزی شده |
| P1 | zstd/brotli | فشرده‌سازی | برنامه‌ریزی شده |
| P1 | libjwt | احراز هویت | برنامه‌ریزی شده |
| P2 | libpng/libjpeg | تصویر | برنامه‌ریزی شده |

</div>

### فازهای پیاده‌سازی

1. **فاز ۱ (فعلی)**: zlib ✅، cJSON، hiredis، OpenSSL، SQLite
2. **فاز ۲**: libcurl، libpq، zstd، brotli، libjwt
3. **فاز ۳**: libpng، libjpeg، libxml2، pcre2
4. **فاز ۴**: libsodium، mongo-c-driver، libuv، کتابخانه‌های جامعه

## تفاوت با WASI

| جنبه | WASI | wali-lib |
|------|------|----------|
| **فلسفه** | API قابل حمل جدید | ABI لینوکس + کتابخانه‌های نیتیو |
| **دسترسی به فایل** | مبتنی بر capability | مستقیم (مانند نیتیو) |
| **کتابخانه‌ها** | باید دوباره پیاده‌سازی شوند | استفاده از کتابخانه‌های نیتیو میزبان |
| **Sandboxing** | داخلی | مبتنی بر اعتماد |

## اعتبارات

- بر اساس [WALI](https://github.com/arjunr2/WALI) - رابط لینوکس WebAssembly
- استفاده از [WAMR](https://github.com/bytecodealliance/wasm-micro-runtime) - محیط اجرای میکرو WebAssembly
- استفاده از [musl](https://musl.libc.org/) - کتابخانه سبک libc

## مجوز

برای جزئیات به [LICENSE](LICENSE) مراجعه کنید.
