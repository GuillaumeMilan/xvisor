# Fault injection software for ZYBO with Xvisor

This project is made to emulate a software running on a ZYBO and 
make a fault injection campaign on it. The objective of the project is to 
inject bit fault in the RAM/Register/FPGA of the ZYBO in order to test the
reliability of the software supposed to run on it. 
This test campaign results are then stored in a file that can be proceed by a 
statistic analysis. 

# How to use the project

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

