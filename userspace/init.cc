#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

int main(void) {
  int pipefd[2];
  char buf;

  pipe(pipefd);

  if (!fork()) {
    while (1) {
      read(pipefd[0], &buf, 1);
      printf("%c", buf);
    }
  } else {
    while (1) {
      char *message = "Hello from child!\n";
      write(pipefd[1], message, strlen(message));
      sleep(1);
    }
  }
}
