#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

int main(void) {
  int file;
  char *buf;

  file = open("testfile.txt", O_RDWR);

  printf("%d\n", file);

  buf = (char *)mmap(nullptr, 4300, PROT_WRITE, MAP_SHARED, file, 4096);

  printf("%c\n", buf[1]);

  for (int i = 0; i < 10; i++) {
    buf[i] = (char)((i % 26) + 65);
  }

  printf("%c\n", buf[1]);

  munmap(buf, 4300);

  close(file);
}
