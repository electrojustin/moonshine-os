#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv) {
  char buf;
  while (1) {
    read(fileno(stdin), &buf, 1);
    printf("%c", buf);
  }
}
