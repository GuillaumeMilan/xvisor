#!/bin/bash

qemu-system-arm -M vexpress-a9 -m 512M -display none -serial stdio -kernel build/vmm.bin -dtb build/arch/arm/board/generic/dts/vexpress/a9/one_guest_vexpress-a9.dtb -initrd build/disk.img
