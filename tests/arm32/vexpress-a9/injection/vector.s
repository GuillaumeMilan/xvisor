.globl _start
_start:
    mov sp,#0x40000000
    bl main
hang: b hang
