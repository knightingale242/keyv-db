#include "common.h"
#include <sys/socket.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>  // Changed from <cerrno>
#include <cstring>
#include <cstdlib>

// Error logging and handling functions
void msg(const char* message) {
    fprintf(stderr, "%s\n", message);
}

void die(const char* message) {
    int err = errno;
    fprintf(stderr, "%s (errno: %d)\n", message, err);
    abort();
}

// Fully read data from file descriptor
int32_t read_full(int fd, char* buf, size_t n) {
    while (n > 0) {
        ssize_t rv = read(fd, buf, n);
        if (rv <= 0) {
            return -1;
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

// Fully write data to file descriptor
int32_t write_full(int fd, const char* buf, size_t n) {
    while (n > 0) {
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0) {
            return -1;
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}