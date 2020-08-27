#include <stdio.h>
#include <unistd.h>

int main(void) {
	if (!fork()) {
		char* argv[] = {"test.exe", "--my_flag", nullptr};
		execve("test.exe", argv, nullptr);
	} else {
		printf("Old process\n");
		while(1) {
			sleep(1);
			printf("Old process\n");
		}
	}
}
