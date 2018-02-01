#!/bin/bash

dtc=./build/tools/dtc/bin/dtc
bindir=./build/disk/images/arm32/vexpress-a9
system_dir=./build/disk/system
img_dir=./build/disk/images/arm32
srcdir=./tests/arm32/vexpress-a9
mkdir -p $system_dir
mkdir -p $bindir
cp -f ./docs/banner/roman.txt $system_dir/banner.txt
cp -f ./docs/logo/xvisor_logo_name.ppm $system_dir/logo.ppm
$dtc -I dts -O dtb -o $img_dir/vexpress-a9x2.dtb $srcdir/vexpress-a9x2.dts
cp -f ./build/$srcdir/freertos/freertos.patched.bin $bindir/freertos.bin
cp -f $srcdir/freertos/nor_flash.list $bindir/nor_flash.list
genext2fs -B 1024 -b 16384 -d ./build/disk ./build/disk.img
