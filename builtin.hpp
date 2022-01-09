#ifndef NYULAN_BUILTIN
#define NYULAN_BUILTIN
#include <cstdint>

#include "nyulan.hpp"
namespace nyulan {
namespace builtin {
std::uint64_t read(int fd, std::uint8_t *buf, std::size_t len);
void write(int fd, std::uint8_t *buf, std::uint64_t len);
int open(char *fname);
void close(int fd);
// void exit(int exit_code);
// void *malloc(std::size_t size);
// void free(Address addr,std::size_t size);
}  // namespace builtin
}  // namespace nyulan
#endif
