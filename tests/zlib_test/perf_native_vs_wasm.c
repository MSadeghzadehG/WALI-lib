#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <zlib.h>

#define WARMUP 2000
#define ITERATIONS 100000
#define DATA_SIZE 4096

int main(void) {
    unsigned char *data = malloc(DATA_SIZE);
    unsigned char *compressed = malloc(DATA_SIZE * 2);
    unsigned char *decompressed = malloc(DATA_SIZE);
    struct timespec start, end;
    double elapsed_ns;
    
    for (size_t i = 0; i < DATA_SIZE; i++) {
        data[i] = (unsigned char)((i * 7) % 256);
    }
    
    uLongf compressed_len = DATA_SIZE * 2;
    compress(compressed, &compressed_len, data, DATA_SIZE);
    
    // Extensive warmup to stabilize CPU frequency
    fprintf(stderr, "Warming up...\n");
    for (int i = 0; i < WARMUP; i++) {
        uLongf len = DATA_SIZE * 2;
        compress(compressed, &len, data, DATA_SIZE);
        len = DATA_SIZE;
        uncompress(decompressed, &len, compressed, compressed_len);
        crc32(0, data, DATA_SIZE);
        adler32(1, data, DATA_SIZE);
    }
    
#ifdef __wasm__
    fprintf(stderr, "Platform: WASM (WALI via iwasm)\n");
#else
    fprintf(stderr, "Platform: Native (x86_64 gcc -O2)\n");
#endif
    fprintf(stderr, "Data: %d bytes, Iterations: %d\n\n", DATA_SIZE, ITERATIONS);
    
    // COMPRESS
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < ITERATIONS; i++) {
        uLongf len = DATA_SIZE * 2;
        compress(compressed, &len, data, DATA_SIZE);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed_ns = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
    fprintf(stderr, "compress:   %7.2f us/call\n", elapsed_ns / ITERATIONS / 1000.0);
    
    // UNCOMPRESS
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < ITERATIONS; i++) {
        uLongf len = DATA_SIZE;
        uncompress(decompressed, &len, compressed, compressed_len);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed_ns = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
    fprintf(stderr, "uncompress: %7.2f us/call\n", elapsed_ns / ITERATIONS / 1000.0);
    
    // CRC32
    volatile uLong crc = 0;
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < ITERATIONS; i++) {
        crc = crc32(crc, data, DATA_SIZE);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed_ns = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
    fprintf(stderr, "crc32:      %7.2f us/call\n", elapsed_ns / ITERATIONS / 1000.0);
    
    // ADLER32
    volatile uLong adler = 1;
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < ITERATIONS; i++) {
        adler = adler32(adler, data, DATA_SIZE);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed_ns = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
    fprintf(stderr, "adler32:    %7.2f us/call\n", elapsed_ns / ITERATIONS / 1000.0);
    
    free(data);
    free(compressed);
    free(decompressed);
    
    return 0;
}
