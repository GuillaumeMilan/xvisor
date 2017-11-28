#! /bin/bash

#Goto directory xvisor-tree
echo "============================="
echo "== Installing Dependencies =="
echo "============================="
xvisor_src=$(pwd)
cd ..

sudo apt-get install flex
sudo apt-get install bison
sudo apt-get install genext2fs
sudo apt-get install telnet

echo "=================================="
echo "== Installing arm-eabi Compiler =="
echo "=================================="
cd tools
wget "http://dn.odroid.in/ODROID-XU/compiler/arm-eabi-4.6.tar.gz"
tar xvf arm-eabi-4.6.tar.gz

export PATH="$PATH":$xvisor_src/../tools/arm-eabi-4.6/bin
export CROSS_COMPILE=arm-eabi-

arm-eabi-gcc --version
tmp=$?
if [[ tmp -ne 0 ]]; then
    echo "Error installing arm-eabi-gcc"
    exit 1
fi

echo "==========================="
echo "== Update Xvisor Sources =="
echo "==========================="
cd $xvisor_src
git submodule init
git submodule update

echo "=========================="
echo "== Compiling for ARM v7 =="
echo "=========================="
make ARCH=arm generic-v7-defconfig -j4
make all dtbs -j4

echo "=========================="
echo "== Setup FreeRTOS Guest =="
echo "=========================="
cd $xvisor_src/..
wget -O FreeRTOSv9.0.0.zip "https://downloads.sourceforge.net/project/freertos/FreeRTOS/V9.0.0/FreeRTOSv9.0.0.zip?r=http%3A%2F%2Fwww.freertos.org%2Fa00104.html&ts=1508233726&use_mirror=vorboss"
unzip FreeRTOSv9.0.0.zip

echo "============================="
echo "== Updating FreeRTOS files =="
echo "============================="
cp -r ./FreeRTOSv9.0.0/FreeRTOS $xvisor_src/tests/arm32/vexpress-a9/freertos
cd $xvisor_src/tests/arm32/vexpress-a9/freertos
mkdir -p port
cp FreeRTOS/Source/portable/GCC/ARM_CA9/* port/
patch -p0 < patches/ports.patch
cp FreeRTOS/Demo/CORTEX_A9_Zynq_ZC702/RTOSDemo/src/FreeRTOSConfig.h .
patch -p0 < patches/freertos-config.patch
export CROSS_COMPILE=arm-eabi-
cp ./FreeRTOS/Source/include/stdint.readme ./stdint.h
cp ./FreeRTOS/Demo/ColdFire_MCF52259_CodeWarrior/stdlib.h ./stdlib.h
make -j4

echo "========================="
echo "== Creating Disk Image =="
echo "========================="
dtc=./build/tools/dtc/bin/dtc
bindir=./build/disk/images/arm32/vexpress-a9
system_dir=./build/disk/system
img_dir=./build/disk/images/arm32
srcdir=./tests/arm32/vexpress-a9
cd $xvisor_src
mkdir -p $system_dir
mkdir -p $bindir
cp -f ./docs/banner/roman.txt $system_dir/banner.txt
cp -f ./docs/logo/xvisor_logo_name.ppm $system_dir/logo.ppm
$dtc -I dts -O dtb -o $img_dir/vexpress-a9x2.dtb $srcdir/vexpress-a9x2.dts
cp -f ./build/$srcdir/freertos/freertos.patched.bin $bindir/freertos.bin
cp -f $srcdir/freertos/nor_flash.list $bindir/nor_flash.list
genext2fs -B 1024 -b 16384 -d ./build/disk ./build/disk.img

echo "==========================="
echo "== Installation Finished =="
echo "==========================="

exit 0
