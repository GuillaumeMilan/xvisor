# Fault injection software for ZYBO with Xvisor
_Authors_:

__Thomas Provent__: Installation and tools choices

__Arthur Dumas, Romain Pierson, Guillaume Milan__: 
Fault injection campaign developpment. 



_Supervisor_: __Michele Portolan__. 

This project is made to emulate a software running on a ZYBO and 
make a fault injection campaign on it. The objective of the project is to 
inject bit fault in the RAM/Register/FPGA of the ZYBO in order to test the
reliability of the software supposed to run on it. 
This test campaign results are then stored in a file that can be proceed by a 
statistic analysis. 

# How to use the project

__Install the project (on Linux) with install.sh__

After cloning the project you have to run the script `install.sh` as sudo user. 
This file will install all the dependencies needed by the project. 

First it will install the tools: 
* flex 
* bison 
* genext2fs
* telnet
* arm-eabi-\*

Then it initialize and update the git submodule. 
It defines the target architecture as ARM. 
It installs FreeRTOS on Xvisor, patch it and create an image disk for Xvisor. 

__Known trouble installing the software__

It exist a problem installing the software on different system than Debian. 
1. __Arch Linux__ 
   Trouble using the make command due to gcc version. 
   (`make ARCH=arm generic-v7-defconfig` and `make all dtbs`). 
   This can be fixed by updating `gcc` and the libraries. To do that execute 
   the following commands: 
       
       sudo packman -Suy gcc gcc-libs

2. __Ubuntu and trouble with libncurse__ 
   This problem also may occured when trying to execute the command 
   `make ARCH=arm generic-v7-defconfig` and `make all dtbs`.
   One of the unstable solution found was to re/install `build-essential` 
   package and install `libncurse`. 
   To do that execute the command: 
       
       sudo apt-get remove build-essential
       sudo apt-get install build-essential
       sudo apt-get install libncurse5-dev 
    
    If the previous solution didn't worked. I suggest you to compile 
    the folder `$GIT_REPOSITORY/tools/openconf` on an other machine and to copy 
    the executable file `conf`. If you find any other way to fix, 
    feel free to contact me at guillaume.milan@grenoble-inp.org.

# Install your software for QEMU emulation.

The script `create_disk_img.sh` is an example how to create a disk image. 

`make dtbs` create the file `build/arch/arm/board/generic/dts/vexpress/a9/one_guest_vexpress-a9.dtb`

Then you need to lauch the QEMU software with the command: 
    
    qemu-system-arm -M vexpress-a9 -m 512M -display none -serial stdio -kernel build/vmm.bin -dtb build/arch/arm/board/generic/dts/vexpress/a9/one_guest_vexpress-a9.dtb -initrd build/disk.img

This command have launched quemu with and Xvisor running on an ARM emulation. 

Then we have to lauch FreeRTOS or any other software linked in disk.img. This is done by kicking the guest0 and bind the UART on the guest. 

    guest kick guest0
    vserial bind guest0/uart

Now the software is running on our Xvisor machine. 
To return on the Xvisor interface, press `ESC`+`Q`+`X`. 

# Install your software on the ZedBoard. 

### Create a boot for Xvisor on Zynq 

__Reference__: 
- [Create a Boot Image on a ZYBO](http://www.wiki.xilinx.com/Getting+Started).
- [\[FR\] Boot Xvisor on a Rpi3](http://www.linuxembedded.fr/2017/03/intro_xvisor/)

The objective of this section is to give a way to build and boot Xvisor with a
guest on an ARM cpu. 

#### Tools 
First we need the following tools:
- [U-Boot](https://www.denx.de/wiki/U-Boot): git://git.denx.de/u-boot-imx.git

- [Petalinux SDK](http://www.wiki.xilinx.com/PetaLinux+Getting+Started): petalinux-v2017.1-final-installer.run __Warning__ This file needs at least 21GB of disk space. 

#### Create a bootable SD card
To create a bootable SD card, it is recommended to follow the step from 
[Create a Boot Image on a ZedBoard](http://www.wiki.xilinx.com/Getting+Started).

You need to part your SD card in 2 part. The first part (`/boot`) will contain:
- `BOOT.bin`: The file on which the ZedBoard will boot. 
- `device_tree.dtb`: at `build/arch/arm/board/generic/dts/vexpress/a9/one_guest_vexpress-a9.dtb`
The second part will contain the files needed for Xvisor and his guest (`/data`)

__BOOT.bin__

If you already posses a bootable ZedBoard (with a SD card containing at least BOOT.bin), it is suggested to modify the u-boot running. Indeed u-boot is very sensitive to the version of the repository. To be able to launch the new u-boot wihtout recreating the __BOOT.bin__ you can launch the `u-boot.elf` directly from the older `u-boot`. This procedure will be describe in detail in the 5th point. 

1. __Create `u-boot.bin`__. To build `u-boot.bin` you need to clone the 
repository at the address: `git://git.denx.de/u-boot-imx.git`. Then build your 
u-boot on the correct architecture. (list available in the folder `configs`).

Define the `$CROSS_COMPILE` system variable to `<peta_SDK_location>/tools/linux-i386/gcc-arm-none-eabi/bin/arm-none-eabi-`

    make <configs_file> 
    make all

Where `<configs_file>` is `zynq_zed_defconfig` for ZedBoard. 
2. Build Xvisor for the Zynq. And then remap the files to the correct address.

    ./u-boot/tools/mkimage -A arm64 -O linux -T kernel -C none -a 0x00080000 -e 0x00080000 -n Xvisor -d $xvisor_src/build/vmm.bin $xvisor_src/build/uvmm.bin
    ./u-boot/tools/mkimage -A arm64 -O linux -T ramdisk -a 0x00000000 -n "Xvisor Ramdisk" -d $xvisor_src/build/disk.img $xvisor_src/build/udisk.img

3. Open Vivado and build `BOOT.bin` as described on [Create a Boot Image on a ZYBO](http://www.wiki.xilinx.com/Getting+Started).


4. Launch the ZedBoard and map uart0 with the command

First list all the device connected to your computer:

    ls /dev/ > <logfile_1>

Then connect the Uart of your ZedBoard  to your computer USB port

    ls /dev/ > <logfile_2>

Get the ID of your ZedBoard to you computer 

    diff <logfile_1> <logfile_2>

For mine it is `/dev/ttyACM0`. You can now connect to the UART of your ZedBoard by entering the command:

    sudo picocom /dev/ttyACM0 -b 115200

A Zynq u-boot terminal is now available. If you want to change the u-boot version with the new elf compiled in the first point, you can do it by loading the file and launching it:

    ext4load mmc 0:2 0x1000 u-boot.elf
    bootelf 0x1000

__Warning this section haven't been successfully done in this project and may not be correct__. 

On the u-boot Zynq terminal you can now type the command to launch the Xvisor software: 

    setenv bootdelay -1
    setenv xvisor "mmc dev 0:0; ext4load mmc 0:2 0x200000 uvmm.bin; ext4load mmc 0:2 0x800000 one_guest_virt-v8.dtb; ext4load mmc 0:2 0x2000000 udisk.img; bootm 0x200000 0x2000000 0x800000"
    saveenv
    run xvisor

# Launch an injection campaign on your software. 

To realize the injeciton campaign we have implemented some command in the file `commands/cmd_guest.c`. To access these commands help you can type:
    
    guest help

__guest status__ get the status of the all the processor of the guest.

__guest loadmem__ load a value in the memory of a guest 

__guest inject__ inject a bit flip to a guest memory address according to the shift value. For example if the shift value is 5, the 5th bits of the value in the memory will be flipped. 

__guest reg__ load a value in the register of both processor of the guest.

__guest reginject__ inject a bit flip to a guest register according to the shift value. `shift(5..0)` define the bit flipped during the operation. And `shift(6)` define in which cpu, the register will be modified. 

__guest stat\_camp__ this command realize an injection campaign on the guest 

- - - - - - - - - - - -
 
# Forked from Xvisor version 0.2.9

http://xvisor.org

http://xhypervisor.org

Please read this document carefully, as it tell you what this is all about,
explain how to build and use the hypervisor, and what to do if something
goes wrong.

## What Is Xvisor?
The term **Xvisor** can stand for: e**X**tensible **v**ersatile hyperv**isor**.

Xvisor is an open-source type-1 hypervisor, which aims at providing
a monolithic, light-weight, portable, and flexible virtualization solution.

It provides a high performance and low memory foot print virtualization
solution for ARMv5, ARMv6, ARMv7a, ARMv7a-ve, ARMv8a, x86_64, and other CPU
architectures.

Xvisor primarily supports Full virtualization hence, supports a wide
range of unmodified Guest operating systems. Paravirtualization is optional
for Xvisor and will be supported in an architecture independent manner
(such as VirtIO PCI/MMIO devices) to ensure no-change in Guest OS for using
paravirtualization.

It has most features expected from a modern hypervisor, such as:

- Device tree based configuration,
- Tickless and high resolution timekeeping,
- Threading framework,
- Host device driver framework,
- IO device emulation framework,
- Runtime loadable modules,
- Pass-through hardware access,
- Dynamic guest creation/destruction,
- Managment terminal,
- Network virtualization,
- Input device virtualization,
- Display device virtualization,
- and many more.

It is distributed under the [GNU General Public License](http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt).
See the accompanying [COPYING](COPYING) file for more details.


## On What Hardware Does It Run?
The Xvisor source code is highly portable and can be easily ported to most
general-purpose 32-bit or 64-bit architectures as long as they have a
paged memory management unit (PMMU) and a port of the GNU C compiler (GCC).

Please refer to the HOSTS text file in top-level directory of source code
for a detailed and formatted list of supported host hardware.


## Documentation
For Xvisor we prefer source level documentation more, so wherever possible
we describe stuff directly in the source code.
This helps us maintain source and its documentation at the same place.

For source level documentation we strictly follow Doxygen style.

Please refer [Doxygen manual](http://www.stack.nl/~dimitri/doxygen/manual.html)
for details.

In addition, we also have various `README` files in the `docs` subdirectory.
Please refer [docs/00-INDEX](docs/00-INDEX) for a list of what is contained in
each file or sub-directory.


## Output Directory
When compiling/configuring hypervisor all output files will by default be
stored in a directory called `build` in hypervisor source directory.

Using the option `make O=<output_dir>` allow you to specify an alternate place
for the output files (including `.config`).

##### Note
If the `O=<output_dir>` option is to be used then it must be used for all
invocations of `make`.


## Configuring
Do not skip this step even if you are only upgrading one minor version.

New configuration options are added in each release, and odd problems will
turn up if the configuration files are not set up as expected.

If you want to *carry your existing configuration to a new version* with
minimal work, use `make oldconfig`, which will only ask you for the answers
to new questions.

To configure hypervisor use one the following command:

	make <configuration_command>
	
or

	make O=<output_dir> <configuration_command>

Various configuration commands (`<configuration_command>`) are:

- `config` - Plain text interface.
- `menuconfig` - Text based color menus, radiolists & dialogs.
- `oldconfig` - Default all questions based on the contents of your existing
	`./.config` file and asking about new config symbols.
- `defconfig` - Create a `./.config` file by using the default values from
	`arch/$ARCH/board/$BOARD/defconfig`.

For configuration Xvisor uses Openconf, which is a modified version of Linux Kconfig.
The motivation behing Openconf is to get Xvisor specific information from
environment variables, and to later extend the syntax of Kconfig to check for
dependent libraries & tools at configuration time.

For more information refer to [Openconf Syntax](tools/openconf/openconf_syntax.txt).


## Compiling
Make sure you have at least gcc 4.x available.

To compile hypervisor use one the following command:

	make

or

	make O=<output_dir>

### Verbose compile/build output
Normally the hypervisor build system runs in a fairly quiet mode (but not totally silent).
However, sometimes you or other hypervisor developers need to see compile,
link, or other commands exactly as they are executed.
For this, use `verbose` build mode by inserting `VERBOSE=y` in the `make` command

	make VERBOSE=y


## Testing
The above steps of configuring and/or compiling are common steps for any
architecture but, this is not sufficient for running hypervisor.
We also need guidelines for configuring/compiling/running a guest OS in
hypervisor environment.
Some guest OS may even expect specific type of hypervisor configuration at
compile time.
Sometimes we may also need to patch a guest OS for proper functioning under
hypervisor environment.

The guidelines required for running a guest OS on a particular type of guest
(Guest CPU + Guest Board) can be found under directory:

	tests/<Guest CPU>/<Guest Board>/README

Please refer to this README for getting detailed information on running a
particular type of OS on particular type of guest in hypervisor.

---

And finally remember

>  It's all JUST FOR FUN....

	.:: HAPPY HACKING ::.

