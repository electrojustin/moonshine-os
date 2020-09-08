#ifndef PROC_UNAME_H
#define PROC_UNAME_H

#include <stdint.h>

namespace proc {

constexpr char *system_name = "MoonshineOS";
constexpr char *node_name = "moonshine1";
constexpr char *release = "5.2.17";
constexpr char *version = "1";
constexpr char *machine = "i386";
constexpr char *domain_name = "moonshine1";

constexpr int UNAME_FIELD_LENGTHS = 65;

struct __attribute__((packed)) new_uname_info {
  char system_name[UNAME_FIELD_LENGTHS];
  char node_name[UNAME_FIELD_LENGTHS];
  char release[UNAME_FIELD_LENGTHS];
  char version[UNAME_FIELD_LENGTHS];
  char machine[UNAME_FIELD_LENGTHS];
  char domain_name[UNAME_FIELD_LENGTHS];
};

uint32_t new_uname(uint32_t name_addr, uint32_t reserved1, uint32_t reserved2,
                   uint32_t reserved3, uint32_t reserved4, uint32_t reserved5);

} // namespace proc

#endif
