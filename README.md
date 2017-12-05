# Fault injection software for ZYBO with Xvisor
_Authors_:

__Thomas Provent__: Installation and tools choices

__Arthur Dumas, Romain Pierson, Guillaume Milan__: 
Fault injection campaign developpment. 


### Create a boot for Xvisor on Zynq 

__Reference__: 
- [Create a Boot Image on a ZYBO](http://www.wiki.xilinx.com/Getting+Started).
- [\[FR\] Boot Xvisor on a Rpi3](http://www.linuxembedded.fr/2017/03/intro_xvisor/)

The objective of this section is to give a way to build and boot Xvisor with a
guest on an ARM cpu. 

#### Tools 
First we need the following tool:
- [U-Boot](https://www.denx.de/wiki/U-Boot): git://git.denx.de/u-boot-imx.git

#### Create a bootable SD card
To create a bootable SD card, it is recommended to follow the step from 
[Create a Boot Image on a ZYBO](http://www.wiki.xilinx.com/Getting+Started).

You need to part your SD card in 2 part. The first part (`/boot`) will contain:
- `BOOT.bin`: The file on which the ZYBO will boot. 
- `device_tree.dtb`: at `build/arch/arm/board/generic/dts/vexpress/a9/one_guest_vexpress-a9.dtb`
The second part will contain the files needed for Xvisor and his guest (`/data`)

__BOOT.bin__

1. __Create `u-boot.bin`__. To build `u-boot.bin` you need to clone the 
repository at the address: `git://git.denx.de/u-boot-imx.git`. Then build your 
u-boot on the correct architecture. (list available in the folder `configs`). 

    make <configs_file>
    make all

2. Build Xvisor for the Zynq. And then remap the files to the correct address.
    ./u-boot/tools/mkimage -A arm64 -O linux -T kernel -C none -a 0x00080000 -e 0x00080000 -n Xvisor -d $xvisor/build/vmm.bin build/uvmm.bin
    ./u-boot/tools/mkimage -A arm64 -O linux -T ramdisk -a 0x00000000 -n "Xvisor Ramdisk" -d $xvisor/build/disk.img build/udisk.img

3. Open Vivado and build `BOOT.bin` as described on [Create a Boot Image on a ZYBO](http://www.wiki.xilinx.com/Getting+Started).


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

First you need to lauch the QEMU software with the command: 
    
    qemu-system-arm -M vexpress-a9 -m 512M -display none -serial stdio -kernel build/vmm.bin -dtb build/arch/arm/board/generic/dts/vexpress/a9/one_guest_vexpress-a9.dtb -initrd build/disk.img

This command have launched quemu with and Xvisor running on an ARM emulation. 

Then we have to lauch FreeRTOS. This is done by kicking the guest0 and bind the UART on the guest. 

    guest kick guest0
    vserial bind guest0/uart

Now FreeRTOS is running on our Xvisor machine. 
To return on the Xvisor interface, press `ESC`+`Q`+`X`. 

__TODO explain how to launch a program under FreeRTOS__

This section must be completed. 

# Install your software on the ZYBO. 

__TODO__

This section must be completed. 

# Launch an injection campaign on your software. 

__TODO__ 

This section must be completed. 

# Analyse the result of the injection. 

__TODO__ 

This section must be completed. 

# Forked from Xvisor version 0.2.xx

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

