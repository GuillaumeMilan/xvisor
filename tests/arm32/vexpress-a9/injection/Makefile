
all: main.bin

main.bin:main.elf
	$(CROSS_COMPILE)objcopy main.elf -O binary main.bin

main.elf:main.o vector.o
	$(CROSS_COMPILE)ld vector.o main.o -T memmap -o main.elf

main.o: injection_test.c
	$(CROSS_COMPILE)gcc -Wall -std=c99 -nostdlib -nostartfiles -ffreestanding -c injection_test.c -o main.o

vector.o: vector.s
	$(CROSS_COMPILE)as vector.s -o vector.o

clean: 
	rm -rf main.bin main.elf vector.o main.o
