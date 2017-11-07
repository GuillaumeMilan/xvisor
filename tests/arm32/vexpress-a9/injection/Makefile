
all: main.bin

main.bin:main.elf
	arm-eabi-objcopy main.elf -O binary main.bin

main.elf:main.o vector.o
	arm-eabi-ld vector.o main.o -T memmap -o main.elf

main.o: injection_test.c
	arm-eabi-gcc -Wall -Werror -std=c99 -nostdlib -nostartfiles -ffreestanding -c injection_test.c -o main.o

vector.o: vector.s
	arm-eabi-as vector.s -o vector.o

clean: 
	rm -rf main.bin main.elf *.o
