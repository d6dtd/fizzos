#include "syscall.h"
#include "fizzio.h"
#include "fizzstr.h"

int main(int argc, char* argv[])
{
    for (int i = 1; i < argc; i++) {
        printf("%s ", argv[i]);
    }
    printf("\n");
}