/*
 * test_zlib.c - Test zlib functionality in WALI/WASM
 * 
 * This program tests basic zlib compression/decompression,
 * CRC32, and Adler32 to verify the native zlib bindings work.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#define TEST_DATA "Hello, WALI! This is a test of zlib compression in WebAssembly. "
#define TEST_REPEAT 10

void print_hex(const unsigned char *data, size_t len) {
    for (size_t i = 0; i < len && i < 32; i++) {
        printf("%02x ", data[i]);
    }
    if (len > 32) printf("...");
    printf("\n");
}

int test_compress_decompress(void) {
    printf("\n=== Test: Compress/Decompress ===\n");
    
    /* Create test data */
    size_t original_len = strlen(TEST_DATA) * TEST_REPEAT;
    char *original = malloc(original_len + 1);
    if (!original) {
        printf("ERROR: Failed to allocate memory\n");
        return 0;
    }
    
    original[0] = '\0';
    for (int i = 0; i < TEST_REPEAT; i++) {
        strcat(original, TEST_DATA);
    }
    
    printf("Original size: %zu bytes\n", original_len);
    
    /* Compress */
    uLong compressed_len = compressBound(original_len);
    unsigned char *compressed = malloc(compressed_len);
    if (!compressed) {
        printf("ERROR: Failed to allocate compressed buffer\n");
        free(original);
        return 0;
    }
    
    int ret = compress(compressed, &compressed_len, 
                       (const unsigned char *)original, original_len);
    if (ret != Z_OK) {
        printf("ERROR: compress() failed with code %d\n", ret);
        free(original);
        free(compressed);
        return 0;
    }
    
    printf("Compressed size: %lu bytes\n", compressed_len);
    printf("Compression ratio: %.1f%%\n", 
           (float)compressed_len / original_len * 100);
    printf("Compressed data: ");
    print_hex(compressed, compressed_len);
    
    /* Decompress */
    uLong decompressed_len = original_len + 1;
    char *decompressed = malloc(decompressed_len);
    if (!decompressed) {
        printf("ERROR: Failed to allocate decompressed buffer\n");
        free(original);
        free(compressed);
        return 0;
    }
    
    uLong src_len = compressed_len;
    ret = uncompress((unsigned char *)decompressed, &decompressed_len,
                     compressed, &src_len);
    if (ret != Z_OK) {
        printf("ERROR: uncompress() failed with code %d\n", ret);
        free(original);
        free(compressed);
        free(decompressed);
        return 0;
    }
    
    printf("Decompressed size: %lu bytes\n", decompressed_len);
    
    /* Verify */
    int success = (decompressed_len == original_len && 
                   memcmp(original, decompressed, original_len) == 0);
    
    if (success) {
        printf("SUCCESS: Original and decompressed data match!\n");
    } else {
        printf("FAILURE: Data mismatch!\n");
    }
    
    free(original);
    free(compressed);
    free(decompressed);
    
    return success;
}

int test_crc32_func(void) {
    printf("\n=== Test: CRC32 ===\n");
    
    const char *data = "Hello, WALI!";
    size_t len = strlen(data);
    
    uLong crc = crc32(0L, Z_NULL, 0);
    crc = crc32(crc, (const unsigned char *)data, len);
    
    printf("Data: \"%s\"\n", data);
    printf("CRC32: %lu (0x%08lx)\n", crc, crc);
    
    /* Known CRC32 for "Hello, WALI!" */
    /* We just verify it returns a non-zero value */
    if (crc != 0) {
        printf("SUCCESS: CRC32 computed successfully\n");
        return 1;
    } else {
        printf("FAILURE: CRC32 returned zero\n");
        return 0;
    }
}

int test_adler32_func(void) {
    printf("\n=== Test: Adler32 ===\n");
    
    const char *data = "Hello, WALI!";
    size_t len = strlen(data);
    
    uLong adler = adler32(0L, Z_NULL, 0);
    adler = adler32(adler, (const unsigned char *)data, len);
    
    printf("Data: \"%s\"\n", data);
    printf("Adler32: %lu (0x%08lx)\n", adler, adler);
    
    /* Adler32 of "Hello, WALI!" should be non-zero */
    if (adler != 0 && adler != 1) {  /* adler32(0, NULL, 0) returns 1 */
        printf("SUCCESS: Adler32 computed successfully\n");
        return 1;
    } else {
        printf("FAILURE: Adler32 returned unexpected value\n");
        return 0;
    }
}

int test_deflate_inflate(void) {
    printf("\n=== Test: Deflate/Inflate Stream ===\n");
    
    const char *input = "WALI zlib stream test - testing deflate and inflate APIs";
    size_t input_len = strlen(input);
    
    printf("Input: \"%s\"\n", input);
    printf("Input size: %zu bytes\n", input_len);
    
    /* Deflate */
    z_stream strm;
    memset(&strm, 0, sizeof(strm));
    
    int ret = deflateInit(&strm, Z_DEFAULT_COMPRESSION);
    if (ret != Z_OK) {
        printf("ERROR: deflateInit() failed with code %d\n", ret);
        return 0;
    }
    
    unsigned char out[256];
    strm.avail_in = input_len;
    strm.next_in = (unsigned char *)input;
    strm.avail_out = sizeof(out);
    strm.next_out = out;
    
    printf("Before deflate: avail_in=%u, avail_out=%u\n", strm.avail_in, strm.avail_out);
    fflush(stdout);
    
    ret = deflate(&strm, Z_FINISH);
    
    printf("After deflate: ret=%d, avail_in=%u, avail_out=%u\n", ret, strm.avail_in, strm.avail_out);
    fflush(stdout);
    
    if (ret != Z_STREAM_END) {
        printf("ERROR: deflate() failed with code %d\n", ret);
        deflateEnd(&strm);
        return 0;
    }
    
    size_t compressed_size = sizeof(out) - strm.avail_out;
    printf("Deflated size: %zu bytes (sizeof(out)=%zu, avail_out=%u)\n", 
           compressed_size, sizeof(out), strm.avail_out);
    
    deflateEnd(&strm);
    
    /* Inflate */
    memset(&strm, 0, sizeof(strm));
    
    ret = inflateInit(&strm);
    if (ret != Z_OK) {
        printf("ERROR: inflateInit() failed with code %d\n", ret);
        return 0;
    }
    
    char decompressed[256];
    strm.avail_in = compressed_size;
    strm.next_in = out;
    strm.avail_out = sizeof(decompressed);
    strm.next_out = (unsigned char *)decompressed;
    
    ret = inflate(&strm, Z_FINISH);
    if (ret != Z_STREAM_END) {
        printf("ERROR: inflate() failed with code %d\n", ret);
        inflateEnd(&strm);
        return 0;
    }
    
    size_t decompressed_size = sizeof(decompressed) - strm.avail_out;
    decompressed[decompressed_size] = '\0';
    
    printf("Inflated size: %zu bytes\n", decompressed_size);
    printf("Inflated: \"%s\"\n", decompressed);
    
    inflateEnd(&strm);
    
    /* Verify */
    if (decompressed_size == input_len && 
        memcmp(input, decompressed, input_len) == 0) {
        printf("SUCCESS: Deflate/Inflate round-trip successful!\n");
        return 1;
    } else {
        printf("FAILURE: Data mismatch after round-trip\n");
        return 0;
    }
}

int main(int argc, char *argv[]) {
    printf("==================================================\n");
    printf("WALI zlib Test Suite\n");
    printf("==================================================\n");
    fflush(stdout);
    
    int passed = 0;
    int total = 4;
    
    passed += test_crc32_func();
    fflush(stdout);
    passed += test_adler32_func();
    fflush(stdout);
    passed += test_compress_decompress();
    fflush(stdout);
    passed += test_deflate_inflate();
    fflush(stdout);
    
    printf("\n==================================================\n");
    printf("Results: %d/%d tests passed\n", passed, total);
    printf("==================================================\n");
    fflush(stdout);
    
    return (passed == total) ? 0 : 1;
}
