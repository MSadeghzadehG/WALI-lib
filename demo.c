#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

int main() {
    printf("[Guest] Starting WALI Demo...\n");

    // 1. File I/O Test
    // This will create a REAL file on your Linux host system
    const char *filename = "wali_output.txt";
    FILE *f = fopen(filename, "w");
    if (!f) {
        perror("Failed to open file");
        return 1;
    }
    
    fprintf(f, "Hello from inside WebAssembly! This file was written via WALI.\n");
    fclose(f);
    printf("[Guest] Wrote to '%s' successfully.\n", filename);

    // 2. Check PID (Process ID)
    // Standard WASI cannot do this. WALI can.
    printf("[Guest] My Process ID is: %d\n", getpid());

    return 0;
}

