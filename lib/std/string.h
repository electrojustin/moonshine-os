#ifndef LIB_STD_STRING_H
#define LIB_STD_STRING_H

#include <stdarg.h>
#include <stdint.h>

namespace lib {
namespace std {

// Formats supported:
// %c = character
// %s = string
// %d = decimal formatted integer
// %x = hex formatted integer (unsigned)
// %% = %
int sprintnk(char* dest, uint32_t max_chars, const char* format, ...);

int vsprintnk(char* dest, uint32_t max_chars, const char* format, va_list parameters);

int strlen(const char* buf);

char streq(const char* string1, const char* string2, uint32_t max_chars);

void to_upper(char* string);

void to_lower(char* string);

int find_first(const char* string, char target, uint32_t max_chars);

char* substring(const char* string, int index1, int index2);

char* strcat(const char* string1, const char* string2);

char* trim(const char* string);

char* make_string_copy(const char* string);

} // namespace std
} // namespace lib

#endif
