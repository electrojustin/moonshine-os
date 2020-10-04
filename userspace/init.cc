#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

int main(void) {
  int pipefd[2];

  pipe(pipefd);

  if (!fork()) {
    dup2(pipefd[0], fileno(stdin));
    char *argv[2] = {"test.exe", nullptr};
    execv("test.exe", argv);
  } else {
    while (1) {
      char *message = "Hello from child!\n";
      write(pipefd[1], message, strlen(message));
      sleep(1);
    }
  }
}
