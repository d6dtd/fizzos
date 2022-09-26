
BUILD_DIR=./build
OBJS=$(BUILD_DIR)/syscall.o $(BUILD_DIR)/fizzio.o $(BUILD_DIR)/fizzstr.o $(BUILD_DIR)/fizzlib.o $(BUILD_DIR)/hello.o

CFLAGS = -c -m32 -fno-stack-protector -I ../include/  -I ../include/kernel/ -I ../include/user/ -I ../include/device/ \
		-I ../include/fs/ -I ../include/shell/ -I ../include/lib/user/ -I ../include/lib/kernel/ -I ../include/lib/ -w

$(BUILD_DIR)/syscall.o : ../lib/user/syscall.c
	gcc $(CFLAGS) $^ -o  $@ 

$(BUILD_DIR)/fizzio.o : ../lib/user/fizzio.c
	gcc $(CFLAGS) $^ -o  $@ 

$(BUILD_DIR)/fizzstr.o : ../lib/fizzstr.c
	gcc $(CFLAGS) $^ -o  $@ 

$(BUILD_DIR)/fizzlib.o : ../lib/fizzlib.c
	gcc $(CFLAGS) $^ -o  $@ 

$(BUILD_DIR)/hello.o : hello.c
	gcc $(CFLAGS) $^ -o  $@ 

$(BUILD_DIR)/hello.bin : $(OBJS)
	ld $^ -o $@ -e main -melf_i386 

.PHONY:
build:$(BUILD_DIR)/hello.bin