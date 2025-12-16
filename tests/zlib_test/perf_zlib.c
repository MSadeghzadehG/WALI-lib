/**
 * zlib Performance Test
 * 
 * This test compares zlib compression/decompression performance.
 * Can be compiled and run both natively and as WASM via iwasm.
 * 
 * Tests:
 * 1. compress/uncompress API (buffer-based)
 * 2. deflate/inflate API (streaming)
 * 3. crc32/adler32 checksums
 * 
 * Compile native:
 *   gcc -O2 -o perf_zlib_native perf_zlib.c -lz
 * 
 * Compile WASM:
 *   ./compile.sh perf_zlib
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <zlib.h>

/* Number of iterations for each test */
#define ITERATIONS 1000
#define SMALL_SIZE 1024          /* 1 KB */
#define MEDIUM_SIZE 65536        /* 64 KB */
#define LARGE_SIZE 1048576       /* 1 MB */

/* Timer utilities */
typedef struct {
    struct timespec start;
    struct timespec end;
} Timer;

static void timer_start(Timer *t) {
    clock_gettime(CLOCK_MONOTONIC, &t->start);
}

static double timer_elapsed_ms(Timer *t) {
    clock_gettime(CLOCK_MONOTONIC, &t->end);
    double start_ms = t->start.tv_sec * 1000.0 + t->start.tv_nsec / 1000000.0;
    double end_ms = t->end.tv_sec * 1000.0 + t->end.tv_nsec / 1000000.0;
    return end_ms - start_ms;
}

/* Generate test data with varying patterns */
static void generate_test_data(unsigned char *buf, size_t size, int pattern) {
    switch (pattern) {
        case 0: /* Highly compressible - repeated pattern */
            for (size_t i = 0; i < size; i++) {
                buf[i] = (unsigned char)(i % 10 + 'A');
            }
            break;
        case 1: /* Medium compressible - text-like */
            for (size_t i = 0; i < size; i++) {
                buf[i] = (unsigned char)((i * 7 + i / 13) % 95 + 32);
            }
            break;
        case 2: /* Low compressibility - pseudo-random */
            for (size_t i = 0; i < size; i++) {
                buf[i] = (unsigned char)((i * 1103515245 + 12345) >> 16);
            }
            break;
        default:
            memset(buf, 'X', size);
    }
}

/* Test 1: compress/uncompress buffer API */
static void test_compress_buffer(size_t data_size, int iterations, const char *label) {
    unsigned char *original = malloc(data_size);
    unsigned char *compressed = malloc(data_size * 2);
    unsigned char *decompressed = malloc(data_size);
    Timer timer;
    double compress_time = 0, decompress_time = 0;
    uLongf compressed_len, decompressed_len;
    int ret;
    
    if (!original || !compressed || !decompressed) {
        printf("  [%s] Memory allocation failed\n", label);
        goto cleanup;
    }
    
    generate_test_data(original, data_size, 1);
    
    /* Warm-up run */
    compressed_len = data_size * 2;
    compress(compressed, &compressed_len, original, data_size);
    decompressed_len = data_size;
    uncompress(decompressed, &decompressed_len, compressed, compressed_len);
    
    /* Benchmark compression */
    timer_start(&timer);
    for (int i = 0; i < iterations; i++) {
        compressed_len = data_size * 2;
        ret = compress(compressed, &compressed_len, original, data_size);
        if (ret != Z_OK) {
            printf("  [%s] Compression failed: %d\n", label, ret);
            goto cleanup;
        }
    }
    compress_time = timer_elapsed_ms(&timer);
    
    /* Benchmark decompression */
    timer_start(&timer);
    for (int i = 0; i < iterations; i++) {
        decompressed_len = data_size;
        ret = uncompress(decompressed, &decompressed_len, compressed, compressed_len);
        if (ret != Z_OK) {
            printf("  [%s] Decompression failed: %d\n", label, ret);
            goto cleanup;
        }
    }
    decompress_time = timer_elapsed_ms(&timer);
    
    /* Verify correctness */
    if (memcmp(original, decompressed, data_size) != 0) {
        printf("  [%s] Data mismatch!\n", label);
        goto cleanup;
    }
    
    double ratio = 100.0 * compressed_len / data_size;
    double compress_throughput = (data_size * iterations / 1024.0 / 1024.0) / (compress_time / 1000.0);
    double decompress_throughput = (data_size * iterations / 1024.0 / 1024.0) / (decompress_time / 1000.0);
    
    printf("  [%s] size=%zu, ratio=%.1f%%, compress=%.2f ms (%.1f MB/s), decompress=%.2f ms (%.1f MB/s)\n",
           label, data_size, ratio, compress_time, compress_throughput, 
           decompress_time, decompress_throughput);

cleanup:
    free(original);
    free(compressed);
    free(decompressed);
}

/* Test 2: deflate/inflate streaming API */
static void test_deflate_stream(size_t data_size, int iterations, const char *label) {
    unsigned char *original = malloc(data_size);
    unsigned char *compressed = malloc(data_size * 2);
    unsigned char *decompressed = malloc(data_size);
    Timer timer;
    double deflate_time = 0, inflate_time = 0;
    z_stream strm;
    int ret;
    
    if (!original || !compressed || !decompressed) {
        printf("  [%s] Memory allocation failed\n", label);
        goto cleanup;
    }
    
    generate_test_data(original, data_size, 1);
    
    size_t compressed_len = 0;
    
    /* Benchmark deflate */
    timer_start(&timer);
    for (int i = 0; i < iterations; i++) {
        memset(&strm, 0, sizeof(strm));
        ret = deflateInit(&strm, Z_DEFAULT_COMPRESSION);
        if (ret != Z_OK) {
            printf("  [%s] deflateInit failed: %d\n", label, ret);
            goto cleanup;
        }
        
        strm.next_in = original;
        strm.avail_in = data_size;
        strm.next_out = compressed;
        strm.avail_out = data_size * 2;
        
        ret = deflate(&strm, Z_FINISH);
        if (ret != Z_STREAM_END) {
            printf("  [%s] deflate failed: %d\n", label, ret);
            deflateEnd(&strm);
            goto cleanup;
        }
        
        compressed_len = strm.total_out;
        deflateEnd(&strm);
    }
    deflate_time = timer_elapsed_ms(&timer);
    
    /* Benchmark inflate */
    timer_start(&timer);
    for (int i = 0; i < iterations; i++) {
        memset(&strm, 0, sizeof(strm));
        ret = inflateInit(&strm);
        if (ret != Z_OK) {
            printf("  [%s] inflateInit failed: %d\n", label, ret);
            goto cleanup;
        }
        
        strm.next_in = compressed;
        strm.avail_in = compressed_len;
        strm.next_out = decompressed;
        strm.avail_out = data_size;
        
        ret = inflate(&strm, Z_FINISH);
        if (ret != Z_STREAM_END) {
            printf("  [%s] inflate failed: %d\n", label, ret);
            inflateEnd(&strm);
            goto cleanup;
        }
        
        inflateEnd(&strm);
    }
    inflate_time = timer_elapsed_ms(&timer);
    
    /* Verify correctness */
    if (memcmp(original, decompressed, data_size) != 0) {
        printf("  [%s] Data mismatch!\n", label);
        goto cleanup;
    }
    
    double ratio = 100.0 * compressed_len / data_size;
    double deflate_throughput = (data_size * iterations / 1024.0 / 1024.0) / (deflate_time / 1000.0);
    double inflate_throughput = (data_size * iterations / 1024.0 / 1024.0) / (inflate_time / 1000.0);
    
    printf("  [%s] size=%zu, ratio=%.1f%%, deflate=%.2f ms (%.1f MB/s), inflate=%.2f ms (%.1f MB/s)\n",
           label, data_size, ratio, deflate_time, deflate_throughput,
           inflate_time, inflate_throughput);

cleanup:
    free(original);
    free(compressed);
    free(decompressed);
}

/* Test 3: crc32/adler32 checksums */
static void test_checksums(size_t data_size, int iterations, const char *label) {
    unsigned char *data = malloc(data_size);
    Timer timer;
    double crc32_time = 0, adler32_time = 0;
    uLong crc_result = 0, adler_result = 0;
    
    if (!data) {
        printf("  [%s] Memory allocation failed\n", label);
        return;
    }
    
    generate_test_data(data, data_size, 2);
    
    /* Benchmark crc32 */
    timer_start(&timer);
    for (int i = 0; i < iterations; i++) {
        crc_result = crc32(0L, Z_NULL, 0);
        crc_result = crc32(crc_result, data, data_size);
    }
    crc32_time = timer_elapsed_ms(&timer);
    
    /* Benchmark adler32 */
    timer_start(&timer);
    for (int i = 0; i < iterations; i++) {
        adler_result = adler32(0L, Z_NULL, 0);
        adler_result = adler32(adler_result, data, data_size);
    }
    adler32_time = timer_elapsed_ms(&timer);
    
    double crc32_throughput = (data_size * iterations / 1024.0 / 1024.0) / (crc32_time / 1000.0);
    double adler32_throughput = (data_size * iterations / 1024.0 / 1024.0) / (adler32_time / 1000.0);
    
    printf("  [%s] size=%zu, crc32=0x%08lx (%.2f ms, %.1f MB/s), adler32=0x%08lx (%.2f ms, %.1f MB/s)\n",
           label, data_size, crc_result, crc32_time, crc32_throughput,
           adler_result, adler32_time, adler32_throughput);
    
    free(data);
}

/* Test different compression levels */
static void test_compression_levels(size_t data_size, int iterations) {
    unsigned char *original = malloc(data_size);
    unsigned char *compressed = malloc(data_size * 2);
    Timer timer;
    
    if (!original || !compressed) {
        printf("  Memory allocation failed\n");
        free(original);
        free(compressed);
        return;
    }
    
    generate_test_data(original, data_size, 1);
    
    printf("  Compression levels (size=%zu, iters=%d):\n", data_size, iterations);
    
    for (int level = 1; level <= 9; level++) {
        uLongf compressed_len;
        double total_time = 0;
        
        timer_start(&timer);
        for (int i = 0; i < iterations; i++) {
            compressed_len = data_size * 2;
            compress2(compressed, &compressed_len, original, data_size, level);
        }
        total_time = timer_elapsed_ms(&timer);
        
        double ratio = 100.0 * compressed_len / data_size;
        double throughput = (data_size * iterations / 1024.0 / 1024.0) / (total_time / 1000.0);
        
        printf("    Level %d: ratio=%.1f%%, time=%.2f ms, throughput=%.1f MB/s\n",
               level, ratio, total_time, throughput);
    }
    
    free(original);
    free(compressed);
}

int main(int argc, char *argv[]) {
    printf("==================================================\n");
    printf("  zlib Performance Test\n");
    printf("  zlib version: %s\n", zlibVersion());
#ifdef __wasm__
    printf("  Platform: WebAssembly (WALI)\n");
#else
    printf("  Platform: Native\n");
#endif
    printf("==================================================\n\n");
    
    /* Test 1: compress/uncompress buffer API */
    printf("Test 1: compress/uncompress API\n");
    test_compress_buffer(SMALL_SIZE, ITERATIONS * 10, "1KB");
    test_compress_buffer(MEDIUM_SIZE, ITERATIONS, "64KB");
    test_compress_buffer(LARGE_SIZE, ITERATIONS / 10, "1MB");
    printf("\n");
    
    /* Test 2: deflate/inflate streaming API */
    printf("Test 2: deflate/inflate streaming API\n");
    test_deflate_stream(SMALL_SIZE, ITERATIONS * 10, "1KB");
    test_deflate_stream(MEDIUM_SIZE, ITERATIONS, "64KB");
    test_deflate_stream(LARGE_SIZE, ITERATIONS / 10, "1MB");
    printf("\n");
    
    /* Test 3: Checksum performance */
    printf("Test 3: Checksum performance (crc32/adler32)\n");
    test_checksums(SMALL_SIZE, ITERATIONS * 100, "1KB");
    test_checksums(MEDIUM_SIZE, ITERATIONS * 10, "64KB");
    test_checksums(LARGE_SIZE, ITERATIONS, "1MB");
    printf("\n");
    
    /* Test 4: Compression levels */
    printf("Test 4: Compression level comparison\n");
    test_compression_levels(MEDIUM_SIZE, ITERATIONS / 10);
    printf("\n");
    
    printf("==================================================\n");
    printf("  Performance test complete!\n");
    printf("==================================================\n");
    
    return 0;
}
