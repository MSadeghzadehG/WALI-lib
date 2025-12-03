#include <stdio.h>
#include <stddef.h>
#include <zlib.h>

int main() {
    printf("sizeof(z_stream) = %zu\n", sizeof(z_stream));
    printf("Offsets:\n");
    printf("  next_in:   %zu\n", offsetof(z_stream, next_in));
    printf("  avail_in:  %zu\n", offsetof(z_stream, avail_in));
    printf("  total_in:  %zu\n", offsetof(z_stream, total_in));
    printf("  next_out:  %zu\n", offsetof(z_stream, next_out));
    printf("  avail_out: %zu\n", offsetof(z_stream, avail_out));
    printf("  total_out: %zu\n", offsetof(z_stream, total_out));
    printf("  msg:       %zu\n", offsetof(z_stream, msg));
    printf("  state:     %zu\n", offsetof(z_stream, state));
    printf("  zalloc:    %zu\n", offsetof(z_stream, zalloc));
    printf("  zfree:     %zu\n", offsetof(z_stream, zfree));
    printf("  opaque:    %zu\n", offsetof(z_stream, opaque));
    printf("  data_type: %zu\n", offsetof(z_stream, data_type));
    printf("  adler:     %zu\n", offsetof(z_stream, adler));
    printf("  reserved:  %zu\n", offsetof(z_stream, reserved));
    fflush(stdout);
    return 0;
}
