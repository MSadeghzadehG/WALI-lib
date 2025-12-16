/**
 * Simple zlib Performance Comparison Test
 * Compares native vs WASM execution of zlib operations
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <zlib.h>

#define ITERATIONS 100
#define DATA_SIZE 65536  /* 64 KB */

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

static void generate_data(unsigned char *buf, size_t size) {
    for (size_t i = 0; i < size; i++) {
        buf[i] = (unsigned char)((i * 7 + i / 13) % 95 + 32);
    }
}

int main(void) {
    unsigned char *original = malloc(DATA_SIZE);
    unsigned char *compressed = malloc(DATA_SIZE * 2);
    unsigned char *decompressed = malloc(DATA_SIZE);
    Timer timer;
    double elapsed;
    int ret;
    
    if (!original || !compressed || !decompressed) {
        printf("Memory allocation failed\n");
        return 1;
    }
    
    generate_data(original, DATA_SIZE);
    
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║              zlib Performance Test Results                   ║\n");
    printf("╠══════════════════════════════════════════════════════════════╣\n");
#ifdef __wasm__
    printf("║  Platform: WebAssembly (WALI via iwasm)                      ║\n");
#else
    printf("║  Platform: Native (gcc)                                      ║\n");
#endif
    printf("║  zlib version: %-46s║\n", zlibVersion());
    printf("║  Data size: %d KB, Iterations: %d                           ║\n", DATA_SIZE/1024, ITERATIONS);
    printf("╚══════════════════════════════════════════════════════════════╝\n\n");
    
    /* Test 1: compress/uncompress */
    printf("Test 1: compress/uncompress API\n");
    printf("─────────────────────────────────\n");
    
    uLongf compressed_len = DATA_SIZE * 2;
    uLongf decompressed_len = DATA_SIZE;
    
    /* Warm up */
    compress(compressed, &compressed_len, original, DATA_SIZE);
    uncompress(decompressed, &decompressed_len, compressed, compressed_len);
    
    /* Benchmark compress */
    timer_start(&timer);
    for (int i = 0; i < ITERATIONS; i++) {
        compressed_len = DATA_SIZE * 2;
        compress(compressed, &compressed_len, original, DATA_SIZE);
    }
    elapsed = timer_elapsed_ms(&timer);
    double compress_throughput = (DATA_SIZE * ITERATIONS / 1024.0 / 1024.0) / (elapsed / 1000.0);
    printf("  compress:   %7.2f ms total, %6.1f MB/s\n", elapsed, compress_throughput);
    
    /* Benchmark uncompress */
    timer_start(&timer);
    for (int i = 0; i < ITERATIONS; i++) {
        decompressed_len = DATA_SIZE;
        uncompress(decompressed, &decompressed_len, compressed, compressed_len);
    }
    elapsed = timer_elapsed_ms(&timer);
    double uncompress_throughput = (DATA_SIZE * ITERATIONS / 1024.0 / 1024.0) / (elapsed / 1000.0);
    printf("  uncompress: %7.2f ms total, %6.1f MB/s\n", elapsed, uncompress_throughput);
    printf("  ratio: %.1f%% (%zu -> %lu bytes)\n\n", 
           100.0 * compressed_len / DATA_SIZE, (size_t)DATA_SIZE, compressed_len);
    
    /* Test 2: deflate/inflate streaming */
    printf("Test 2: deflate/inflate streaming API\n");
    printf("──────────────────────────────────────\n");
    
    z_stream strm;
    size_t deflated_len = 0;
    
    /* Benchmark deflate */
    timer_start(&timer);
    for (int i = 0; i < ITERATIONS; i++) {
        memset(&strm, 0, sizeof(strm));
        deflateInit(&strm, Z_DEFAULT_COMPRESSION);
        strm.next_in = original;
        strm.avail_in = DATA_SIZE;
        strm.next_out = compressed;
        strm.avail_out = DATA_SIZE * 2;
        deflate(&strm, Z_FINISH);
        deflated_len = strm.total_out;
        deflateEnd(&strm);
    }
    elapsed = timer_elapsed_ms(&timer);
    double deflate_throughput = (DATA_SIZE * ITERATIONS / 1024.0 / 1024.0) / (elapsed / 1000.0);
    printf("  deflate:    %7.2f ms total, %6.1f MB/s\n", elapsed, deflate_throughput);
    
    /* Benchmark inflate */
    timer_start(&timer);
    for (int i = 0; i < ITERATIONS; i++) {
        memset(&strm, 0, sizeof(strm));
        inflateInit(&strm);
        strm.next_in = compressed;
        strm.avail_in = deflated_len;
        strm.next_out = decompressed;
        strm.avail_out = DATA_SIZE;
        inflate(&strm, Z_FINISH);
        inflateEnd(&strm);
    }
    elapsed = timer_elapsed_ms(&timer);
    double inflate_throughput = (DATA_SIZE * ITERATIONS / 1024.0 / 1024.0) / (elapsed / 1000.0);
    printf("  inflate:    %7.2f ms total, %6.1f MB/s\n\n", elapsed, inflate_throughput);
    
    /* Test 3: Checksums */
    printf("Test 3: Checksum performance\n");
    printf("────────────────────────────\n");
    
    uLong crc_result = 0, adler_result = 0;
    
    timer_start(&timer);
    for (int i = 0; i < ITERATIONS * 10; i++) {
        crc_result = crc32(0L, original, DATA_SIZE);
    }
    elapsed = timer_elapsed_ms(&timer);
    double crc_throughput = (DATA_SIZE * ITERATIONS * 10 / 1024.0 / 1024.0) / (elapsed / 1000.0);
    printf("  crc32:      %7.2f ms total, %6.1f MB/s (0x%08lx)\n", elapsed, crc_throughput, crc_result);
    
    timer_start(&timer);
    for (int i = 0; i < ITERATIONS * 10; i++) {
        adler_result = adler32(0L, original, DATA_SIZE);
    }
    elapsed = timer_elapsed_ms(&timer);
    double adler_throughput = (DATA_SIZE * ITERATIONS * 10 / 1024.0 / 1024.0) / (elapsed / 1000.0);
    printf("  adler32:    %7.2f ms total, %6.1f MB/s (0x%08lx)\n\n", elapsed, adler_throughput, adler_result);
    
    /* Verify correctness */
    if (memcmp(original, decompressed, DATA_SIZE) == 0) {
        printf("✓ Data integrity verified\n");
    } else {
        printf("✗ Data mismatch!\n");
    }
    
    free(original);
    free(compressed);
    free(decompressed);
    
    return 0;
}
