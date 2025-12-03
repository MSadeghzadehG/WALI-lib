/*
 * test_simple.c - Minimal zlib test 
 */

#include <stdio.h>
#include <string.h>
#include <zlib.h>

int main(int argc, char *argv[]) {
    printf("Starting simple zlib test...\n");
    fflush(stdout);
    
    /* Test CRC32 */
    const char *data = "Hello";
    uLong crc = crc32(0L, Z_NULL, 0);
    printf("Initial CRC: %lu\n", crc);
    fflush(stdout);
    
    crc = crc32(crc, (const unsigned char *)data, 5);
    printf("CRC32 of 'Hello': %lu (0x%08lx)\n", crc, crc);
    fflush(stdout);
    
    /* Test Adler32 */
    uLong adler = adler32(0L, Z_NULL, 0);
    printf("Initial Adler: %lu\n", adler);
    fflush(stdout);
    
    adler = adler32(adler, (const unsigned char *)data, 5);
    printf("Adler32 of 'Hello': %lu (0x%08lx)\n", adler, adler);
    fflush(stdout);
    
    printf("Simple test completed!\n");
    fflush(stdout);
    return 0;
}
