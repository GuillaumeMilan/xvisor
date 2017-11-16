#include <arm_io.h>
#include <arm_irq.h>
#include <arm_plat.h>
#include <arm_board.h>
#include <arm_timer.h>
#include <arm_stdio.h>


void make_seg_fault(void) {
    int *ptr = (int*)-1;
    *ptr = 0x12345678;
    arm_puts("I survived\n");
    arm_printf("PTR : 0x%x\n",(unsigned int)ptr);
    arm_printf("VAL : 0x%x\n",(unsigned int)*ptr);
}

int is_hexa(char* c){
    if (c[0]=='0' && c[1]=='x')
        return 1;
    else 
        return 0;
}
unsigned int convert_to_uint(char* c) {
    unsigned int value=0;
    int index = 2;
    while (c[index] != '\0') {
        if (c[index] >= 'a' && c[index] <= 'f') 
            value = value + 10 + c[index] - 'a';
        else if (c[index] >= '0' && c[index] <= '9') 
            value = value + c[index] - '0';
        else 
            arm_printf("Non hexa value detected 0 is returned!");
        value = value << 4;
        index++;
    }
    return value;
}

int change_mem() {
    int *address;
    int value;
    char command[255];
    arm_printf("Address : ");
    arm_gets(command,255,'\n');
    if (! is_hexa(command))
        return -1;
    arm_printf("Converting ...\n");
    address = (int*) convert_to_uint(command);
    arm_printf("Value : ");
    arm_gets(command,255,'\n');
    if (! is_hexa(command))
        return -1;
    value = (int) convert_to_uint(command);
    *address = value;
    return 0;
}

int increment_loop(void) {
    int i = 0;
    arm_puts("Welcome to FreeRTOS!\n");
    while(1) {
        if (i%100 == 0) {
            arm_printf("i : 0x%x = 0x%x\n", (unsigned int)&i,(unsigned int)i);
        }
        i++;
        if (i>0x00ffffff) {
            arm_puts("Exiting Now");                                
            return 0;
        }
    }
}
