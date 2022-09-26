#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

void f(){
	write(1, "666\n", 4);
}

int main(void)
{	
	atexit(f);
	printf("123");
}
