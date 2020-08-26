#include <stdio.h>
#include <unistd.h>

int main(void) {
	if (!fork()) {
		execve("test.exe", nullptr, nullptr);
	} else {
		printf("Old process\n");
		while(1) {
			sleep(1);
			printf("Old process\n");
		}
	}
}
