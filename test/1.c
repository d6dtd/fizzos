#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

char s[100000];

int main(void){
	int fd = open("./build/hello.bin", O_RDWR);
	int count = read(fd, s, 100000);
	printf("count:%d\n", count);
	printf("{");
	for (int i = 0; i < count; i++) {
		printf("%d, ", s[i]);
		if ((i + 1) % 1000 == 0) {
			printf("\n");
		}
	}
	printf("}");
}
