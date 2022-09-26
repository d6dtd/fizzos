#!/bin/sh

BIN=$1

CFLAGS="-Wall -c -fno-builtin -W -Wstrict-prototypes -Wmissing-prototypes -Wsystem-headers \
        -I ../include/lib -I ../include/lib/user -m32"

OBJS="../build/fizzio.o ../build/syscall.o ../build/fizzstr.o ../build/fizzlib.o ./start.o"

gcc  $CFLAGS -o "./build/"$BIN".o" $BIN".c"
ld -melf_i386 "./build/"$BIN".o" $OBJS -o "./build/"$BIN
SEC_CNT=$(wc -c "./build/"$BIN | cut -d ' ' -f 1)

if [ $SEC_CNT != "0" ];then
    dd if=./build/$BIN of=../hd60M.img bs=512 count=$SEC_CNT seek=$2 conv=notrunc
fi