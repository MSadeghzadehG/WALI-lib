/*
 * test_gzip.c - Test gzip file I/O functionality in WALI/WASM
 * 
 * This program tests the complete gzip file I/O API including
 * gzopen, gzread, gzwrite, gzseek, gztell, etc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#define TEST_FILE "test_output.gz"
#define TEST_DATA "Hello, WALI gzip! Testing gzip file I/O in WebAssembly.\n"
#define TEST_REPEAT 100

static int tests_passed = 0;
static int tests_total = 0;

#define TEST(name) printf("\n=== Test: %s ===\n", name); tests_total++;
#define PASS() printf("PASSED\n"); tests_passed++;
#define FAIL(msg) printf("FAILED: %s\n", msg); return 0;

int test_gzwrite_gzread(void) {
    TEST("gzwrite/gzread basic");
    
    /* Write test data */
    gzFile wf = gzopen(TEST_FILE, "wb");
    if (!wf) {
        FAIL("gzopen for write failed");
    }
    
    int written = gzwrite(wf, TEST_DATA, strlen(TEST_DATA));
    if (written != (int)strlen(TEST_DATA)) {
        gzclose(wf);
        FAIL("gzwrite returned wrong count");
    }
    
    if (gzclose(wf) != Z_OK) {
        FAIL("gzclose failed after write");
    }
    
    /* Read back */
    gzFile rf = gzopen(TEST_FILE, "rb");
    if (!rf) {
        FAIL("gzopen for read failed");
    }
    
    char buf[256];
    int read_bytes = gzread(rf, buf, sizeof(buf) - 1);
    if (read_bytes != (int)strlen(TEST_DATA)) {
        printf("Expected %zu, got %d\n", strlen(TEST_DATA), read_bytes);
        gzclose(rf);
        FAIL("gzread returned wrong count");
    }
    
    buf[read_bytes] = '\0';
    if (strcmp(buf, TEST_DATA) != 0) {
        printf("Expected: '%s'\nGot: '%s'\n", TEST_DATA, buf);
        gzclose(rf);
        FAIL("Data mismatch");
    }
    
    gzclose(rf);
    PASS();
    return 1;
}

int test_gzputs_gzgets(void) {
    TEST("gzputs/gzgets");
    
    const char *lines[] = {
        "Line 1: Hello\n",
        "Line 2: World\n",
        "Line 3: WALI\n"
    };
    int nlines = sizeof(lines) / sizeof(lines[0]);
    
    /* Write lines */
    gzFile wf = gzopen(TEST_FILE, "wb");
    if (!wf) {
        FAIL("gzopen for write failed");
    }
    
    for (int i = 0; i < nlines; i++) {
        if (gzputs(wf, lines[i]) < 0) {
            gzclose(wf);
            FAIL("gzputs failed");
        }
    }
    gzclose(wf);
    
    /* Read lines */
    gzFile rf = gzopen(TEST_FILE, "rb");
    if (!rf) {
        FAIL("gzopen for read failed");
    }
    
    char buf[256];
    for (int i = 0; i < nlines; i++) {
        if (gzgets(rf, buf, sizeof(buf)) == NULL) {
            gzclose(rf);
            FAIL("gzgets returned NULL");
        }
        if (strcmp(buf, lines[i]) != 0) {
            printf("Line %d: expected '%s', got '%s'\n", i, lines[i], buf);
            gzclose(rf);
            FAIL("Line mismatch");
        }
    }
    
    gzclose(rf);
    PASS();
    return 1;
}

int test_gzputc_gzgetc(void) {
    TEST("gzputc/gzgetc");
    
    const char *data = "ABCDEFGHIJ";
    int len = strlen(data);
    
    /* Write chars */
    gzFile wf = gzopen(TEST_FILE, "wb");
    if (!wf) {
        FAIL("gzopen for write failed");
    }
    
    for (int i = 0; i < len; i++) {
        if (gzputc(wf, data[i]) != data[i]) {
            gzclose(wf);
            FAIL("gzputc failed");
        }
    }
    gzclose(wf);
    
    /* Read chars */
    gzFile rf = gzopen(TEST_FILE, "rb");
    if (!rf) {
        FAIL("gzopen for read failed");
    }
    
    for (int i = 0; i < len; i++) {
        int c = gzgetc(rf);
        if (c != data[i]) {
            printf("Char %d: expected '%c' (%d), got '%c' (%d)\n", 
                   i, data[i], data[i], c, c);
            gzclose(rf);
            FAIL("Char mismatch");
        }
    }
    
    /* Should be at EOF */
    if (gzgetc(rf) != -1) {
        gzclose(rf);
        FAIL("Expected EOF");
    }
    
    gzclose(rf);
    PASS();
    return 1;
}

int test_gzseek_gztell(void) {
    TEST("gzseek/gztell");
    
    const char *data = "0123456789ABCDEFGHIJ";
    int len = strlen(data);
    
    /* Write data */
    gzFile wf = gzopen(TEST_FILE, "wb");
    if (!wf) {
        FAIL("gzopen for write failed");
    }
    gzwrite(wf, data, len);
    gzclose(wf);
    
    /* Test seeking */
    gzFile rf = gzopen(TEST_FILE, "rb");
    if (!rf) {
        FAIL("gzopen for read failed");
    }
    
    /* Read a few bytes */
    char buf[4];
    gzread(rf, buf, 4);
    
    /* Check position */
    z_off_t pos = gztell(rf);
    if (pos != 4) {
        printf("Expected pos 4, got %lld\n", (long long)pos);
        gzclose(rf);
        FAIL("gztell wrong after read");
    }
    
    /* Seek to position 10 */
    if (gzseek(rf, 10, SEEK_SET) != 10) {
        gzclose(rf);
        FAIL("gzseek SEEK_SET failed");
    }
    
    /* Read at position 10 */
    int c = gzgetc(rf);
    if (c != 'A') {
        printf("Expected 'A' at position 10, got '%c'\n", c);
        gzclose(rf);
        FAIL("Wrong char at position 10");
    }
    
    /* Rewind */
    if (gzrewind(rf) != 0) {
        gzclose(rf);
        FAIL("gzrewind failed");
    }
    
    /* Should be at start */
    c = gzgetc(rf);
    if (c != '0') {
        printf("Expected '0' after rewind, got '%c'\n", c);
        gzclose(rf);
        FAIL("Wrong char after rewind");
    }
    
    gzclose(rf);
    PASS();
    return 1;
}

int test_gzeof(void) {
    TEST("gzeof");
    
    const char *data = "Short";
    
    /* Write */
    gzFile wf = gzopen(TEST_FILE, "wb");
    if (!wf) {
        FAIL("gzopen for write failed");
    }
    gzwrite(wf, data, strlen(data));
    gzclose(wf);
    
    /* Read until EOF */
    gzFile rf = gzopen(TEST_FILE, "rb");
    if (!rf) {
        FAIL("gzopen for read failed");
    }
    
    /* Not at EOF yet */
    if (gzeof(rf)) {
        gzclose(rf);
        FAIL("gzeof returned true before reading");
    }
    
    /* Read all data */
    char buf[256];
    gzread(rf, buf, sizeof(buf));
    
    /* Should be at EOF now */
    if (!gzeof(rf)) {
        gzclose(rf);
        FAIL("gzeof returned false at end");
    }
    
    gzclose(rf);
    PASS();
    return 1;
}

int test_gzerror(void) {
    TEST("gzerror");
    
    /* Write valid file first */
    gzFile wf = gzopen(TEST_FILE, "wb");
    if (!wf) {
        FAIL("gzopen for write failed");
    }
    gzwrite(wf, "Test", 4);
    gzclose(wf);
    
    /* Open for read */
    gzFile rf = gzopen(TEST_FILE, "rb");
    if (!rf) {
        FAIL("gzopen for read failed");
    }
    
    /* No error initially */
    int errnum;
    const char *msg = gzerror(rf, &errnum);
    if (errnum != Z_OK) {
        printf("Initial error: %s (%d)\n", msg, errnum);
        gzclose(rf);
        FAIL("Error before any operation");
    }
    
    gzclose(rf);
    PASS();
    return 1;
}

int test_gzungetc(void) {
    TEST("gzungetc");
    
    const char *data = "ABCD";
    
    /* Write */
    gzFile wf = gzopen(TEST_FILE, "wb");
    if (!wf) {
        FAIL("gzopen for write failed");
    }
    gzwrite(wf, data, strlen(data));
    gzclose(wf);
    
    /* Read with ungetc */
    gzFile rf = gzopen(TEST_FILE, "rb");
    if (!rf) {
        FAIL("gzopen for read failed");
    }
    
    int c = gzgetc(rf);  /* Read 'A' */
    if (c != 'A') {
        gzclose(rf);
        FAIL("First getc failed");
    }
    
    /* Push back 'X' */
    if (gzungetc('X', rf) != 'X') {
        gzclose(rf);
        FAIL("gzungetc failed");
    }
    
    /* Should read 'X' now */
    c = gzgetc(rf);
    if (c != 'X') {
        printf("Expected 'X' after ungetc, got '%c'\n", c);
        gzclose(rf);
        FAIL("Did not get ungotten char");
    }
    
    /* Then 'B' */
    c = gzgetc(rf);
    if (c != 'B') {
        printf("Expected 'B', got '%c'\n", c);
        gzclose(rf);
        FAIL("Wrong next char");
    }
    
    gzclose(rf);
    PASS();
    return 1;
}

int test_large_file(void) {
    TEST("Large file (1MB)");
    
    /* Create 1MB of compressible data */
    size_t size = 1024 * 1024;
    char *data = malloc(size);
    if (!data) {
        FAIL("Failed to allocate test data");
    }
    
    for (size_t i = 0; i < size; i++) {
        data[i] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"[i % 26];
    }
    
    /* Write */
    gzFile wf = gzopen(TEST_FILE, "wb");
    if (!wf) {
        free(data);
        FAIL("gzopen for write failed");
    }
    
    int written = gzwrite(wf, data, size);
    if (written != (int)size) {
        printf("Wrote %d of %zu bytes\n", written, size);
        gzclose(wf);
        free(data);
        FAIL("gzwrite incomplete");
    }
    gzclose(wf);
    
    /* Read back */
    gzFile rf = gzopen(TEST_FILE, "rb");
    if (!rf) {
        free(data);
        FAIL("gzopen for read failed");
    }
    
    char *buf = malloc(size);
    if (!buf) {
        gzclose(rf);
        free(data);
        FAIL("Failed to allocate read buffer");
    }
    
    int total_read = 0;
    while (total_read < (int)size) {
        int n = gzread(rf, buf + total_read, size - total_read);
        if (n <= 0) break;
        total_read += n;
    }
    
    if (total_read != (int)size) {
        printf("Read %d of %zu bytes\n", total_read, size);
        gzclose(rf);
        free(data);
        free(buf);
        FAIL("gzread incomplete");
    }
    
    /* Verify */
    if (memcmp(data, buf, size) != 0) {
        gzclose(rf);
        free(data);
        free(buf);
        FAIL("Data mismatch");
    }
    
    gzclose(rf);
    free(data);
    free(buf);
    
    PASS();
    return 1;
}

int test_compression_levels(void) {
    TEST("Compression levels");
    
    const char *test_data = "This is test data that will be compressed at different levels. "
                            "The quick brown fox jumps over the lazy dog. "
                            "Pack my box with five dozen liquor jugs. ";
    size_t data_len = strlen(test_data) * 10;
    char *data = malloc(data_len + 1);
    if (!data) {
        FAIL("Failed to allocate");
    }
    
    data[0] = '\0';
    for (int i = 0; i < 10; i++) {
        strcat(data, test_data);
    }
    
    printf("Original size: %zu bytes\n", data_len);
    
    /* Test different compression levels */
    int levels[] = {1, 6, 9};
    for (int i = 0; i < 3; i++) {
        char mode[8];
        sprintf(mode, "wb%d", levels[i]);
        
        gzFile wf = gzopen(TEST_FILE, mode);
        if (!wf) {
            free(data);
            FAIL("gzopen failed");
        }
        
        gzwrite(wf, data, data_len);
        gzclose(wf);
        
        /* Get compressed size by opening and checking offset */
        FILE *f = fopen(TEST_FILE, "rb");
        if (f) {
            fseek(f, 0, SEEK_END);
            long compressed_size = ftell(f);
            fclose(f);
            printf("Level %d: %ld bytes (%.1f%%)\n", 
                   levels[i], compressed_size, 
                   100.0 * compressed_size / data_len);
        }
    }
    
    free(data);
    PASS();
    return 1;
}

int main(int argc, char *argv[]) {
    printf("==================================================\n");
    printf("WALI Gzip File I/O Test Suite\n");
    printf("==================================================\n");
    
    tests_passed += test_gzwrite_gzread();
    tests_passed += test_gzputs_gzgets();
    tests_passed += test_gzputc_gzgetc();
    tests_passed += test_gzseek_gztell();
    tests_passed += test_gzeof();
    tests_passed += test_gzerror();
    tests_passed += test_gzungetc();
    tests_passed += test_large_file();
    tests_passed += test_compression_levels();
    
    printf("\n==================================================\n");
    printf("Results: %d/%d tests passed\n", tests_passed, tests_total);
    printf("==================================================\n");
    
    /* Clean up test file */
    remove(TEST_FILE);
    
    return (tests_passed == tests_total) ? 0 : 1;
}
