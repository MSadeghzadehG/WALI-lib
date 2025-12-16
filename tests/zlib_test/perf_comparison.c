/**
 * zlib Performance Comparison: Native vs WASM
 * Tests compress/uncompress and checksum APIs
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <zlib.h>

#define ITERATIONS 100
#define DATA_SIZE 65536  /* 64 KB */

static double get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
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
    double start, elapsed;
    
    if (!original || !compressed || !decompressed) {
        printf("Memory allocation failed\n");
        return 1;
    }
    
    generate_data(original, DATA_SIZE);
    
    printf("=== zlib Performance Test ===\n");
#ifdef __wasm__
    printf("Platform: WASM (WALI)\n");
#else
    printf("Platform: Native\n");
#endif
    printf("Data: %d KB x %d iterations\n\n", DATA_SIZE/1024, ITERATIONS);
    
    uLongf compressed_len = DATA_SIZE * 2;
    uLongf decompressed_len = DATA_SIZE;
    
    /* Warm up */
    compress(compressed, &compressed_len, original, DATA_SIZE);
    uncompress(decompressed, &decompressed_len, compressed, compressed_len);
    
    /* ===== Test 1: compress ===== */
    start = get_time_ms();
    for (int i = 0; i < ITERATIONS; i++) {
        compressed_len = DATA_SIZE * 2;
        compress(compressed, &compressed_len, original, DATA_SIZE);
    }
    elapsed = get_time_ms() - start;
    double compress_mbps = (DATA_SIZE * ITERATIONS / 1024.0 / 1024.0) / (elapsed / 1000.0);
    printf("compress:     %7.2f ms  %7.1f MB/s\n", elapsed, compress_mbps);
    
    /* ===== Test 2: uncompress ===== */
    start = get_time_ms();
    for (int i = 0; i < ITERATIONS; i++) {
        decompressed_len = DATA_SIZE;
        uncompress(decompressed, &decompressed_len, compressed, compressed_len);
    }
    elapsed = get_time_ms() - start;
    double uncompress_mbps = (DATA_SIZE * ITERATIONS / 1024.0 / 1024.0) / (elapsed / 1000.0);
    printf("uncompress:   %7.2f ms  %7.1f MB/s\n", elapsed, uncompress_mbps);
    
    /* ===== Test 3: crc32 ===== */
    uLong crc = 0;
    start = get_time_ms();
    for (int i = 0; i < ITERATIONS * 10; i++) {
        crc = crc32(0L, original, DATA_SIZE);
    }
    elapsed = get_time_ms() - start;
    double crc_mbps = (DATA_SIZE * ITERATIONS * 10 / 1024.0 / 1024.0) / (elapsed / 1000.0);
    printf("crc32:        %7.2f ms  %7.1f MB/s\n", elapsed, crc_mbps);
    
    /* ===== Test 4: adler32 ===== */
    uLong adler = 0;
    start = get_time_ms();
    for (int i = 0; i < ITERATIONS * 10; i++) {
        adler = adler32(0L, original, DATA_SIZE);
    }
    elapsed = get_time_ms() - start;
    double adler_mbps = (DATA_SIZE * ITERATIONS * 10 / 1024.0 / 1024.0) / (elapsed / 1000.0);
    printf("adler32:      %7.2f ms  %7.1f MB/s\n", elapsed, adler_mbps);
    
    printf("\nCompression: %zu -> %lu bytes (%.1f%%)\n", 
           (size_t)DATA_SIZE, compressed_len, 100.0 * compressed_len / DATA_SIZE);
    
    /* Verify */
    if (memcmp(original, decompressed, DATA_SIZE) == 0) {
        printf("Integrity: PASSED\n");
    } else {
        printf("Integrity: FAILED!\n");
    }
    
    free(original);
    free(compressed);
    free(decompressed);
    
    return 0;
}
