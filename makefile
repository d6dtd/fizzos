
OBJS=build/main.o build/init.o build/print.o build/interrupt.o  build/kernel.o build/syscall-init.o \
	 build/debug.o build/memory.o build/bitmap.o build/list.o build/syscall.o build/fizzlib.o \
	 build/fizzstr.o build/printk.o build/timer.o build/kthread.o build/switch.o build/sync.o \
	 build/console.o build/keyboard.o build/ioqueue.o build/tss.o build/process.o build/ide.o \
	 build/fs.o build/dir.o build/file.o build/inode.o build/fizzio.o build/usercopy.o \
	 build/print2.o build/print1.o build/namei.o build/page.o build/buddy.o build/exec.o build/vma.o	\
	 build/fork.o build/exit.o

CFLAGS = -c -m32 -fno-stack-protector -Wall -fno-builtin -W -Wstrict-prototypes -Wmissing-prototypes \
		-I include/  -I include/kernel/ -I include/user/ -I include/device/ -I include/fs/ \
	    -I include/lib/user/ -I include/lib/kernel/ -I include/lib/

# device/
build/console.o : device/console.c
	gcc $(CFLAGS) $^ -o  $@ 

build/ide.o : device/ide.c
	gcc $(CFLAGS) $^ -o  $@ 

build/ioqueue.o : device/ioqueue.c
	gcc $(CFLAGS) $^ -o  $@ 

build/keyboard.o : device/keyboard.c
	gcc $(CFLAGS) $^ -o  $@ 

build/timer.o : device/timer.c
	gcc $(CFLAGS) $^ -o  $@ 

# fs/
build/dir.o : fs/dir.c
	gcc $(CFLAGS) $^ -o  $@ 

build/file.o : fs/file.c
	gcc $(CFLAGS) $^ -o  $@ 

build/fs.o : fs/fs.c
	gcc $(CFLAGS) $^ -o  $@ 

build/inode.o : fs/inode.c
	gcc $(CFLAGS) $^ -o  $@ 
 
build/namei.o : fs/namei.c
	gcc $(CFLAGS) $^ -o  $@ 

# kernel/
build/debug.o : kernel/debug.c
	gcc $(CFLAGS) $^ -o  $@ 

build/exec.o : kernel/exec.c
	gcc $(CFLAGS) $^ -o  $@ 

build/exit.o : kernel/exit.c
	gcc $(CFLAGS) $^ -o  $@ 

build/fork.o : kernel/fork.c
	gcc $(CFLAGS) $^ -o  $@ 

build/init.o : kernel/init.c
	gcc $(CFLAGS) $^ -o  $@ 

build/interrupt.o : kernel/interrupt.c 
	gcc $(CFLAGS) $^ -o  $@ 

build/kernel.o : kernel/kernel.S
	nasm -f elf $^ -o $@

build/kthread.o : kernel/kthread.c
	gcc $(CFLAGS) $^ -o  $@ 

build/main.o : kernel/main.c
	gcc $(CFLAGS) $^ -o  $@ 

build/memory.o : kernel/memory.c
	gcc $(CFLAGS) $^ -o  $@ 

build/buddy.o : kernel/buddy.c
	gcc $(CFLAGS) $^ -o  $@ 

build/page.o : kernel/page.S
	nasm -f elf $^ -o $@

build/switch.o : kernel/switch.S
	nasm -f elf $^ -o $@

build/sync.o : kernel/sync.c
	gcc $(CFLAGS) $^ -o  $@ 

build/vma.o : kernel/vma.c
	gcc $(CFLAGS) $^ -o  $@ 

# lib/kernel/
build/bitmap.o : lib/kernel/bitmap.c
	gcc $(CFLAGS) $^ -o  $@

build/list.o : lib/kernel/list.c
	gcc $(CFLAGS) $^ -o  $@

build/print.o : lib/kernel/print.S
	nasm -f elf $^ -o $@

build/print2.o : lib/kernel/print2.S
	nasm -f elf $^ -o $@

build/print1.o : lib/kernel/print1.c
	gcc $(CFLAGS) $^ -o  $@

build/printk.o : lib/kernel/printk.c
	gcc $(CFLAGS) $^ -o  $@

# lib/user/
build/fizzio.o : lib/user/fizzio.c
	gcc $(CFLAGS) $^ -o  $@

build/syscall.o : lib/user/syscall.c
	gcc $(CFLAGS) $^ -o  $@

# lib/
build/fizzlib.o : lib/fizzlib.c
	gcc $(CFLAGS) $^ -o  $@

build/fizzstr.o : lib/fizzstr.c
	gcc $(CFLAGS) $^ -o  $@

# user/
build/process.o : user/process.c
	gcc $(CFLAGS) $^ -o  $@ 

build/syscall-init.o : user/syscall-init.c
	gcc $(CFLAGS) $^ -o  $@ 

build/tss.o : user/tss.c
	gcc $(CFLAGS) $^ -o  $@ 

build/usercopy.o : user/usercopy.c
	gcc $(CFLAGS) $^ -o  $@ 

build/kernel.bin : $(OBJS)
	ld $^ -Ttext 0xc0001500 -o $@ -e main -melf_i386 

.PHONY: dd all clean build

build : build/kernel.bin

dd:build/kernel.bin
	dd if=boot/mbr.bin of=./hd60M.img bs=512 count=1 conv=notrunc
	dd if=boot/loader.bin of=./hd60M.img bs=512 count=4 seek=2 conv=notrunc
	dd if=build/kernel.bin of=./hd60M.img bs=512 count=300 seek=9 conv=notrunc

all:dd

clean:
	rm build/*

rebuild: clean all
