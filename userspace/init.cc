#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

int main(void) {
	int ret = mkdir("testdirtestdirtestdir", 0777);

	chdir("testdirtestdirtestdir");

	DIR* dir = opendir(".");
	struct dirent* entry;
	while(entry = readdir(dir)) {
		printf("%s\n", entry->d_name);
	}
	closedir(dir);

	printf("next dir\n");

	dir = opendir("..");
	while(entry = readdir(dir)) {
		printf("%s\n", entry->d_name);
	}
	closedir(dir);
}
