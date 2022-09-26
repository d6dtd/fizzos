#include <unistd.h>
#include <stdio.h>
int main(){
	int count = write(1, "666\n", 4);
	count = printf("666");
	printf("\n%d\n", count);
}
