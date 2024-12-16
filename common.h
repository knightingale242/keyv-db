#ifndef COMMON_H
#define COMMON_H

#include <cstdint>
#include <cstddef>
#include <cstdio>  // For FILE* and fprintf

// Maximum message length
inline constexpr size_t k_max_msg = 4096;
inline constexpr size_t k_max_args = 1024;

// Utility functions for error handling and logging
void msg(const char* message);
void die(const char* message);

// Network I/O functions with full read/write
int32_t read_full(int fd, char* buf, size_t n);
int32_t write_full(int fd, const char* buf, size_t n);

#endif // COMMON_H