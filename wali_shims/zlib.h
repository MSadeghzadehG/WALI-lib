/* wali_shims/zlib.h
 * WASM shim header for zlib compression library
 * Complete zlib support for WALI including gzip file I/O
 */

#ifndef WALI_ZLIB_H_
#define WALI_ZLIB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

/* ===== Version and constants ===== */
#define ZLIB_VERSION "1.3"
#define ZLIB_VERNUM 0x1300

/* Compression levels */
#define Z_NO_COMPRESSION      0
#define Z_BEST_SPEED          1
#define Z_DEFAULT_COMPRESSION (-1)
#define Z_BEST_COMPRESSION    9

/* Flush values */
#define Z_NO_FLUSH      0
#define Z_PARTIAL_FLUSH 1
#define Z_SYNC_FLUSH    2
#define Z_FULL_FLUSH    3
#define Z_FINISH        4
#define Z_BLOCK         5
#define Z_TREES         6

/* Compression method */
#define Z_DEFLATED 8

/* Strategy */
#define Z_DEFAULT_STRATEGY    0
#define Z_FILTERED            1
#define Z_HUFFMAN_ONLY        2
#define Z_RLE                 3
#define Z_FIXED               4

/* Return codes */
#define Z_OK            0
#define Z_STREAM_END    1
#define Z_NEED_DICT     2
#define Z_ERRNO        (-1)
#define Z_STREAM_ERROR (-2)
#define Z_DATA_ERROR   (-3)
#define Z_MEM_ERROR    (-4)
#define Z_BUF_ERROR    (-5)
#define Z_VERSION_ERROR (-6)

/* Compression window bits */
#define MAX_WBITS 15
#define DEF_MEM_LEVEL 8
#define MAX_MEM_LEVEL 9

/* Data type hints */
#define Z_BINARY   0
#define Z_TEXT     1
#define Z_ASCII    Z_TEXT
#define Z_UNKNOWN  2

/* Null pointer */
#define Z_NULL 0

/* ===== Data types ===== */
typedef unsigned char Byte;
typedef Byte Bytef;
typedef unsigned int uInt;
typedef unsigned long uLong;
typedef uLong uLongf;
typedef void *voidp;
typedef void *voidpf;
typedef const void *voidpc;
typedef size_t z_size_t;

/* Offset type for gzip file positioning */
typedef int64_t z_off_t;
typedef z_off_t z_off64_t;

/* z_stream structure */
struct z_stream_s {
    const Byte *next_in;     /* next input byte */
    uint32_t    avail_in;    /* number of bytes available at next_in */
    uLong       total_in;    /* total nb of input bytes read so far */

    Byte       *next_out;    /* next output byte will go here */
    uint32_t    avail_out;   /* remaining free space at next_out */
    uLong       total_out;   /* total nb of bytes output so far */

    const char *msg;         /* last error message, NULL if no error */
    struct internal_state *state; /* not visible by applications */

    void       *zalloc;      /* used to allocate the internal state */
    void       *zfree;       /* used to free the internal state */
    voidp       opaque;      /* private data object passed to zalloc and zfree */

    int         data_type;   /* best guess about the data type */
    uLong       adler;       /* adler32 value of the uncompressed data */
    uLong       reserved;    /* reserved for future use */
};

typedef struct z_stream_s z_stream;
typedef z_stream *z_streamp;

/* gz_header structure for gzip header information */
typedef struct gz_header_s {
    int         text;        /* true if compressed data believed to be text */
    uLong       time;        /* modification time */
    int         xflags;      /* extra flags (not used when writing) */
    int         os;          /* operating system */
    Bytef      *extra;       /* pointer to extra field or Z_NULL */
    uInt        extra_len;   /* extra field length (valid if extra != Z_NULL) */
    uInt        extra_max;   /* space at extra (only when reading header) */
    Bytef      *name;        /* pointer to zero-terminated file name or Z_NULL */
    uInt        name_max;    /* space at name (only when reading header) */
    Bytef      *comment;     /* pointer to zero-terminated comment or Z_NULL */
    uInt        comm_max;    /* space at comment (only when reading header) */
    int         hcrc;        /* true if there was or will be a header crc */
    int         done;        /* true when done reading gzip header */
} gz_header;

typedef gz_header *gz_headerp;

/* gzFile is a handle (uint32_t) in WALI, not a pointer */
typedef uint32_t gzFile;

/* ===== Basic compression/decompression functions ===== */

__attribute__((import_module("env"), import_name("wali_compressBound")))
uLong compressBound(uLong sourceLen);

__attribute__((import_module("env"), import_name("wali_compress")))
int compress(Bytef *dest, uLongf *destLen,
             const Bytef *source, uLong sourceLen);

__attribute__((import_module("env"), import_name("wali_compress2")))
int compress2(Bytef *dest, uLongf *destLen,
              const Bytef *source, uLong sourceLen, int level);

__attribute__((import_module("env"), import_name("wali_uncompress")))
int uncompress(Bytef *dest, uLongf *destLen,
               const Bytef *source, uLong sourceLen);

__attribute__((import_module("env"), import_name("wali_uncompress2")))
int uncompress2(Bytef *dest, uLongf *destLen,
                const Bytef *source, uLong *sourceLen);

/* ===== Deflate functions ===== */

__attribute__((import_module("env"), import_name("wali_deflateInit_")))
int deflateInit_(z_streamp strm, int level,
                 const char *version, int stream_size);

__attribute__((import_module("env"), import_name("wali_deflateInit2_")))
int deflateInit2_(z_streamp strm, int level, int method,
                  int windowBits, int memLevel, int strategy,
                  const char *version, int stream_size);

#define deflateInit(strm, level) \
        deflateInit_((strm), (level), ZLIB_VERSION, (int)sizeof(z_stream))

#define deflateInit2(strm, level, method, windowBits, memLevel, strategy) \
        deflateInit2_((strm), (level), (method), (windowBits), (memLevel), \
                      (strategy), ZLIB_VERSION, (int)sizeof(z_stream))

__attribute__((import_module("env"), import_name("wali_deflate")))
int deflate(z_streamp strm, int flush);

__attribute__((import_module("env"), import_name("wali_deflateEnd")))
int deflateEnd(z_streamp strm);

__attribute__((import_module("env"), import_name("wali_deflateSetDictionary")))
int deflateSetDictionary(z_streamp strm,
                         const Bytef *dictionary, uInt dictLength);

__attribute__((import_module("env"), import_name("wali_deflateGetDictionary")))
int deflateGetDictionary(z_streamp strm,
                         Bytef *dictionary, uInt *dictLength);

__attribute__((import_module("env"), import_name("wali_deflateCopy")))
int deflateCopy(z_streamp dest, z_streamp source);

__attribute__((import_module("env"), import_name("wali_deflateReset")))
int deflateReset(z_streamp strm);

__attribute__((import_module("env"), import_name("wali_deflateParams")))
int deflateParams(z_streamp strm, int level, int strategy);

__attribute__((import_module("env"), import_name("wali_deflateTune")))
int deflateTune(z_streamp strm,
                int good_length, int max_lazy, int nice_length, int max_chain);

__attribute__((import_module("env"), import_name("wali_deflateBound")))
uLong deflateBound(z_streamp strm, uLong sourceLen);

__attribute__((import_module("env"), import_name("wali_deflatePending")))
int deflatePending(z_streamp strm, unsigned *pending, int *bits);

__attribute__((import_module("env"), import_name("wali_deflatePrime")))
int deflatePrime(z_streamp strm, int bits, int value);

__attribute__((import_module("env"), import_name("wali_deflateSetHeader")))
int deflateSetHeader(z_streamp strm, gz_headerp head);

__attribute__((import_module("env"), import_name("wali_deflateResetKeep")))
int deflateResetKeep(z_streamp strm);

/* ===== Inflate functions ===== */

__attribute__((import_module("env"), import_name("wali_inflateInit_")))
int inflateInit_(z_streamp strm,
                 const char *version, int stream_size);

__attribute__((import_module("env"), import_name("wali_inflateInit2_")))
int inflateInit2_(z_streamp strm, int windowBits,
                  const char *version, int stream_size);

#define inflateInit(strm) \
        inflateInit_((strm), ZLIB_VERSION, (int)sizeof(z_stream))

#define inflateInit2(strm, windowBits) \
        inflateInit2_((strm), (windowBits), ZLIB_VERSION, (int)sizeof(z_stream))

__attribute__((import_module("env"), import_name("wali_inflate")))
int inflate(z_streamp strm, int flush);

__attribute__((import_module("env"), import_name("wali_inflateEnd")))
int inflateEnd(z_streamp strm);

__attribute__((import_module("env"), import_name("wali_inflateSetDictionary")))
int inflateSetDictionary(z_streamp strm,
                         const Bytef *dictionary, uInt dictLength);

__attribute__((import_module("env"), import_name("wali_inflateGetDictionary")))
int inflateGetDictionary(z_streamp strm,
                         Bytef *dictionary, uInt *dictLength);

__attribute__((import_module("env"), import_name("wali_inflateCopy")))
int inflateCopy(z_streamp dest, z_streamp source);

__attribute__((import_module("env"), import_name("wali_inflateReset")))
int inflateReset(z_streamp strm);

__attribute__((import_module("env"), import_name("wali_inflateReset2")))
int inflateReset2(z_streamp strm, int windowBits);

__attribute__((import_module("env"), import_name("wali_inflatePrime")))
int inflatePrime(z_streamp strm, int bits, int value);

__attribute__((import_module("env"), import_name("wali_inflateSync")))
int inflateSync(z_streamp strm);

__attribute__((import_module("env"), import_name("wali_inflateMark")))
long inflateMark(z_streamp strm);

__attribute__((import_module("env"), import_name("wali_inflateGetHeader")))
int inflateGetHeader(z_streamp strm, gz_headerp head);

__attribute__((import_module("env"), import_name("wali_inflateSyncPoint")))
int inflateSyncPoint(z_streamp strm);

__attribute__((import_module("env"), import_name("wali_inflateValidate")))
int inflateValidate(z_streamp strm, int check);

__attribute__((import_module("env"), import_name("wali_inflateCodesUsed")))
unsigned long inflateCodesUsed(z_streamp strm);

__attribute__((import_module("env"), import_name("wali_inflateResetKeep")))
int inflateResetKeep(z_streamp strm);

/* inflateBack functions - require callbacks, not yet implemented */
__attribute__((import_module("env"), import_name("wali_inflateBackInit_")))
int inflateBackInit_(z_streamp strm, int windowBits,
                     unsigned char *window,
                     const char *version, int stream_size);

#define inflateBackInit(strm, windowBits, window) \
        inflateBackInit_((strm), (windowBits), (window), \
                         ZLIB_VERSION, (int)sizeof(z_stream))

__attribute__((import_module("env"), import_name("wali_inflateBack")))
int inflateBack(z_streamp strm,
                unsigned (*in)(void *, unsigned char **),
                void *in_desc,
                int (*out)(void *, unsigned char *, unsigned),
                void *out_desc);

__attribute__((import_module("env"), import_name("wali_inflateBackEnd")))
int inflateBackEnd(z_streamp strm);

/* ===== Utility functions ===== */

__attribute__((import_module("env"), import_name("wali_zlibVersion")))
const char *zlibVersion(void);

__attribute__((import_module("env"), import_name("wali_zlibCompileFlags")))
uLong zlibCompileFlags(void);

__attribute__((import_module("env"), import_name("wali_zError")))
const char *zError(int err);

__attribute__((import_module("env"), import_name("wali_adler32")))
uLong adler32(uLong adler, const Bytef *buf, uInt len);

__attribute__((import_module("env"), import_name("wali_adler32_z")))
uLong adler32_z(uLong adler, const Bytef *buf, z_size_t len);

__attribute__((import_module("env"), import_name("wali_adler32_combine")))
uLong adler32_combine(uLong adler1, uLong adler2, z_off_t len2);

__attribute__((import_module("env"), import_name("wali_crc32")))
uLong crc32(uLong crc, const Bytef *buf, uInt len);

__attribute__((import_module("env"), import_name("wali_crc32_z")))
uLong crc32_z(uLong crc, const Bytef *buf, z_size_t len);

__attribute__((import_module("env"), import_name("wali_crc32_combine")))
uLong crc32_combine(uLong crc1, uLong crc2, z_off_t len2);

__attribute__((import_module("env"), import_name("wali_crc32_combine_gen")))
uLong crc32_combine_gen(z_off_t len2);

__attribute__((import_module("env"), import_name("wali_crc32_combine_op")))
uLong crc32_combine_op(uLong crc1, uLong crc2, uLong op);

/* ===== Gzip file I/O functions ===== */

__attribute__((import_module("env"), import_name("wali_gzopen")))
gzFile gzopen(const char *path, const char *mode);

__attribute__((import_module("env"), import_name("wali_gzdopen")))
gzFile gzdopen(int fd, const char *mode);

__attribute__((import_module("env"), import_name("wali_gzbuffer")))
int gzbuffer(gzFile file, unsigned size);

__attribute__((import_module("env"), import_name("wali_gzsetparams")))
int gzsetparams(gzFile file, int level, int strategy);

__attribute__((import_module("env"), import_name("wali_gzread")))
int gzread(gzFile file, voidp buf, unsigned len);

__attribute__((import_module("env"), import_name("wali_gzfread")))
z_size_t gzfread(voidp buf, z_size_t size, z_size_t nitems, gzFile file);

__attribute__((import_module("env"), import_name("wali_gzwrite")))
int gzwrite(gzFile file, voidpc buf, unsigned len);

__attribute__((import_module("env"), import_name("wali_gzfwrite")))
z_size_t gzfwrite(voidpc buf, z_size_t size, z_size_t nitems, gzFile file);

__attribute__((import_module("env"), import_name("wali_gzputs")))
int gzputs(gzFile file, const char *s);

__attribute__((import_module("env"), import_name("wali_gzgets")))
char *gzgets(gzFile file, char *buf, int len);

__attribute__((import_module("env"), import_name("wali_gzputc")))
int gzputc(gzFile file, int c);

__attribute__((import_module("env"), import_name("wali_gzgetc")))
int gzgetc(gzFile file);

__attribute__((import_module("env"), import_name("wali_gzungetc")))
int gzungetc(int c, gzFile file);

__attribute__((import_module("env"), import_name("wali_gzflush")))
int gzflush(gzFile file, int flush);

__attribute__((import_module("env"), import_name("wali_gzseek")))
z_off_t gzseek(gzFile file, z_off_t offset, int whence);

__attribute__((import_module("env"), import_name("wali_gzrewind")))
int gzrewind(gzFile file);

__attribute__((import_module("env"), import_name("wali_gztell")))
z_off_t gztell(gzFile file);

__attribute__((import_module("env"), import_name("wali_gzoffset")))
z_off_t gzoffset(gzFile file);

__attribute__((import_module("env"), import_name("wali_gzeof")))
int gzeof(gzFile file);

__attribute__((import_module("env"), import_name("wali_gzdirect")))
int gzdirect(gzFile file);

__attribute__((import_module("env"), import_name("wali_gzclose")))
int gzclose(gzFile file);

__attribute__((import_module("env"), import_name("wali_gzclose_r")))
int gzclose_r(gzFile file);

__attribute__((import_module("env"), import_name("wali_gzclose_w")))
int gzclose_w(gzFile file);

__attribute__((import_module("env"), import_name("wali_gzerror")))
const char *gzerror(gzFile file, int *errnum);

__attribute__((import_module("env"), import_name("wali_gzclearerr")))
void gzclearerr(gzFile file);

/* 64-bit variants (same as regular on WALI since z_off_t is 64-bit) */
#define gzseek64 gzseek
#define gztell64 gztell
#define gzoffset64 gzoffset
#define gzopen64 gzopen

#ifdef __cplusplus
}
#endif

#endif /* WALI_ZLIB_H_ */
