#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv) {
  printf("Hello, world! %d %s\n", argc, argv[argc - 1]);
  int *my_ptr = nullptr;
  *my_ptr = 1234;
  printf("Goodbye, world!\n");
}
