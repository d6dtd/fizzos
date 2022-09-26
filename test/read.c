#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
char s[1025];
int main(){
	int fd = open("./build/hello.bin", O_RDWR, 0777);
	int count;
	printf("{");
	while ((count = read(fd, s, 1024)) == 1024) {
		for (int i = 0; i < count; i++) {
			printf("%d, ", s[i]);
		}
		printf("\n");
	}
	for (int i = 0; i < count; i++) {
		printf("%d, ", s[i]);
	}
	printf("}");
	printf("\n");
}
