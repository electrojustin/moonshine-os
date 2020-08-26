#include <stdio.h>
#include <unistd.h>

int main(void) {
	printf("Hello, world!\n");
	int* my_ptr = nullptr;
	*my_ptr = 1234;
	printf("Goodbye, world!\n");
}
