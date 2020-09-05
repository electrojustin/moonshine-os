#include <stdio.h>
#include <unistd.h>

int main(void) {
	FILE* file;
	char buf[200];

	file = fopen("testfile.txt", "r+");

	fseek(file, 4000, SEEK_SET);
	
	fgets(buf, 200, file);

	printf("%c\n", buf[0]);

	for (int i = 0; i < 200; i++) {
		buf[i] = (char)((i % 26) + 65);
	}

	printf("%c\n", buf[0]);

	fseek(file, 4000, SEEK_SET);

	fwrite(buf, 1, 200, file);

	fclose(file);
}
