#include <stdarg.h>

#include "lib/std/memory.h"
#include "lib/std/stdio.h"
#include "lib/std/string.h"

namespace lib {
namespace std {

namespace {

constexpr char hexits[17] = "0123456789ABCDEF";

// Helper function for converting decimal numbers to strings.
int convert_decimal(char *dest, int32_t to_print) {
  int written = 0;
  int index = 9;
  uint8_t digits[10] = {0};
  if (to_print < 0) {
    dest[written] = '-';
    written++;
    to_print *= -1;
  } else if (!to_print) {
    dest[written] = '0';
    return 1;
  }
  while (to_print) {
    digits[index] = to_print % 10;
    to_print /= 10;
    index--;
  }
  index++;
  for (index; index < 10; index++) {
    dest[written] = hexits[digits[index]];
    written++;
  }
  return written;
}

// Helper function for converting hexadecimal numbers to strings.
int convert_hex(char *dest, uint32_t to_print) {
  *dest = '0';
  dest++;
  *dest = 'x';
  dest++;
  for (int i = 28; i >= 0; i -= 4) {
    *dest = hexits[(to_print >> i) & 0xF];
    dest++;
  }
  return 10;
}

} // namespace

int sprintnk(char *dest, uint32_t max_chars, const char *format, ...) {
  va_list parameters;
  va_start(parameters, format);
  return vsprintnk(dest, max_chars, format, parameters);
}

int vsprintnk(char *dest, uint32_t max_chars, const char *format,
              va_list parameters) {
  int index = 0;
  int written = 0;
  while (format[index]) {
    if (written + 1 >= max_chars) {
      return -1;
    }

    if (format[index] != '%') {
      dest[written] = format[index];
      written++;
      index++;
      continue;
    }

    index++;
    if (written + 1 >= max_chars || !format[index]) {
      return -1;
    }

    char *string_buf = nullptr;
    switch (format[index]) {
    case '%':
      dest[written] = '%';
      written++;
      break;
    case 'c':
      dest[written] = (char)va_arg(parameters, int);
      written++;
      break;
    case 's':
      string_buf = (char *)va_arg(parameters, char *);
      while (*string_buf) {
        if (written >= max_chars) {
          return -1;
        }
        dest[written] = *string_buf;
        written++;
        string_buf++;
      }
      break;
    case 'd':
      if (written + 11 >= max_chars) {
        return -1;
      }
      written += convert_decimal(dest + written, va_arg(parameters, int32_t));
      break;
    case 'x':
      if (written + 11 >= max_chars) {
        return -1;
      }
      written += convert_hex(dest + written, va_arg(parameters, uint32_t));
      break;
    default:
      return -1;
    }
    index++;
  }

  dest[written] = 0;
  written++;

  return written;
}

int strlen(const char *buf) {
  int ret = 0;
  while (buf[ret]) {
    ret++;
  }
  return ret;
}

char streq(const char *string1, const char *string2, uint32_t max_chars) {
  for (int i = 0; i < max_chars; i++) {
    if (!string1[i] && !string2[i]) {
      return 1;
    } else if (string1[i] && string2[i]) {
      if (string1[i] != string2[i]) {
        return 0;
      }
    } else {
      return 0;
    }
  }

  return 1;
}

void to_upper(char *string) {
  while (*string) {
    if (*string >= 'a' && *string <= 'z') {
      *string -= 32;
    }
    string++;
  }
}

void to_lower(char *string) {
  while (*string) {
    if (*string >= 'A' && *string <= 'Z') {
      *string += 32;
    }
    string++;
  }
}

int find_first(const char *string, char target, uint32_t max_chars) {
  for (int i = 0; i < max_chars && string[i]; i++) {
    if (string[i] == target) {
      return i;
    }
  }

  return -1;
}

char *make_string_copy(const char *string) {
  int len = strlen(string) + 1;
  char *ret = (char *)kmalloc(len);
  int i = 0;

  for (i; i < len; i++) {
    ret[i] = string[i];
  }
  ret[i] = 0;

  return ret;
}

char *substring(const char *string, int index1, int index2) {
  char *ret = (char *)kmalloc(index2 - index1 + 1);
  int i;
  for (i = 0; i < index2 - index1; i++) {
    ret[i] = string[i + index1];
  }

  ret[i] = 0;

  return ret;
}

char *strcat(const char *string1, const char *string2) {
  int len1 = strlen(string1);
  int len2 = strlen(string2);
  char *ret = (char *)kmalloc(len1 + len2 + 1);
  int i;
  for (i = 0; i < len1; i++) {
    ret[i] = string1[i];
  }
  for (i; i - len1 < len2; i++) {
    ret[i] = string2[i - len1];
  }
  ret[i] = 0;

  return ret;
}

namespace {

const char whitespace[] = {'\n', '\t', '\r', ' '};

} // namespace

char *trim(const char *string) {
  int length = strlen(string);
  int i = 0;
  int j = length - 1;
  int k;

  for (i; i < length; i++) {
    for (k = 0; k < sizeof(whitespace); k++) {
      if (string[i] == whitespace[k]) {
        break;
      }
    }
    if (k == sizeof(whitespace)) {
      break;
    }
  }

  for (j; j >= 0; j--) {
    for (k = 0; k < sizeof(whitespace); k++) {
      if (string[j] == whitespace[k]) {
        break;
      }
    }
    if (k == sizeof(whitespace)) {
      break;
    }
  }

  if (i <= j) {
    return substring(string, i, j + 1);
  }

  return make_string_copy(string);
}

} // namespace std
} // namespace lib
