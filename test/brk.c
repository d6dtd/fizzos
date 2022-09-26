#include <unistd.h>
#include <stdio.h>

int main(){
	printf("%p\n", sbrk(0));
	printf("%d\n", brk(100));
	
}
