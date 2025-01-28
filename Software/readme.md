# Pogobot Software

<!-- vscode-markdown-toc -->
- [Pogobot Software](#pogobot-software)
  - [Folder contains](#folder-contains)
  - [First consideration](#first-consideration)
  - [Software API](#software-api)
  - [Software Installation (for expert mode)](#software-installation-for-expert-mode)
    - [Repository clone](#repository-clone)
    - [Install all the dependency](#install-all-the-dependency)
    - [Adding your user to Dialout group](#adding-your-user-to-dialout-group)
    - [Compile gateware and pogobios (Has to be done once)](#compile-gateware-and-pogobios-has-to-be-done-once)
    - [Compile the SDK (Has to be done once too)](#compile-the-sdk-has-to-be-done-once-too)
    - [Compile and test "HelloWorld"](#compile-and-test-helloworld)
    - [Compile and upload your application](#compile-and-upload-your-application)
    - [Upload a empty or faulty robot](#upload-a-empty-or-faulty-robot)
    - [Memory map](#memory-map)
    - [Install on Linux distributions other than Ubuntu by using Singularity Containers](#install-on-linux-distributions-other-than-ubuntu-by-using-singularity-containers)
  - [Tools](#tools)
    - [version\_creation\_litex.sh](#version_creation_litexsh)
    - [Doc generation](#doc-generation)
  - [Kwown Issues](#kwown-issues)
    - [yosys-abc](#yosys-abc)
    - [nextpnr](#nextpnr)

<!-- vscode-markdown-toc-config
	numbering=false
	autoSave=true
	/vscode-markdown-toc-config -->
<!-- /vscode-markdown-toc -->

## Folder contains
This folder contains all the code needed to all the object in this project.

The folder :
- "binary_installation" contains all the binary at different version and the script to install them.
- "example" contains the example for the robot (software)
- "platforms" contains the python code for the different plateform used (gateware)
- "pogobios" contains the all the code of the bios on each robot when you start them (software)
- "pogobject" contains the code for the esp8266 and the code of the webapp to control them
- "pogolib" contains the code of the library that will used to program the robots (software)
- "pogoWallApp" contains the code of a webapp that help to control the wall
- "rtl" contains the python code for the different plateform used (gateware)
- "targets" contains the python code for the different plateform used (gateware)


## <a name='first-consideration'></a>First consideration 
In order to avoid to much complexity for the pogobot usage, there is to way to program a pogobot : the simple mode and the expert mode.

- The simple mode allows you to program the user part of the pogobot.
- The expert mode gives you access of all the code and allows the programmer to change every aspect of the robots. 

Please seriously consider to start by the simple mode to avoid difficult installation by going [here](https://github.com/nekonaute/pogobot-sdk)   


## <a name='software-api'></a>Software API
The robot API is described inside [pogodocs.md](../pogodocs.md). <br> 
Most of the information is contained inside pogolib/pogobot.h but if you want to go deeper, the best is to browse pogolib and pogobios. 


## <a name='software-installation-(for-expert-mode)'></a>Software Installation (for expert mode)

The Software is divided between the code for the FPGA (gateware) and the code for the softcore (software). 

:warning: Everything was tested with Ubuntu 20.04

### <a name='repository-clone'></a>Repository clone
    git clone https://github.com/nekonaute/pogobot.git
    cd pogobot


### <a name='install-all-the-dependency'></a>Install all the dependency

Create dependencies directory

    mkdir -p dependencies

**system dependency**

The list is adapted for Ubuntu distribution

    sudo apt-get -y install bison build-essential clang clang-format cmake flex gawk git graphviz libboost-all-dev libboost-dev libboost-filesystem-dev libboost-iostreams-dev libboost-program-options-dev libboost-python-dev libboost-system-dev libboost-thread-dev libeigen3-dev libffi-dev libftdi-dev libreadline-dev mercurial pkg-config python python3 python3-dev python3-pip python3-setuptools qt5-default tcl-dev xdot zlib1g-dev

    sudo pip3 install meson==0.64.1 ninja
    echo "export PATH=\"\$PATH:~/.local/bin\"" >>~/.bashrc

**litex + migen**

From https://github.com/enjoy-digital/litex

    mkdir -p dependencies/litex
    cd dependencies/litex
    wget https://raw.githubusercontent.com/enjoy-digital/litex/master/litex_setup.py
    chmod +x litex_setup.py
    ./litex_setup.py init 

    cat ../../litex_version.txt | awk '{ if(!system("test -d " $1)) {system("git -C " $1 " checkout " $3)}}'

    sudo ./litex_setup.py install

Recover the Cross Compiler

    wget https://static.dev.sifive.com/dev-tools/freedom-tools/v2020.08/riscv64-unknown-elf-gcc-10.1.0-2020.08.2-x86_64-linux-ubuntu14.tar.gz
    tar xvf riscv64-unknown-elf-gcc-10.1.0-2020.08.2-x86_64-linux-ubuntu14.tar.gz

    export GCC_RISCV=$PWD/riscv64-unknown-elf-gcc-10.1.0-2020.08.2-x86_64-linux-ubuntu14/bin/
    echo "export PATH=\"\$PATH:$GCC_RISCV\"" >>~/.bashrc


Notice the option `update` of litex_setup.py have a high probability to break the project.

**icestorm**

From [Icestorm installation
instructions](http://www.clifford.at/icestorm/#install):


    mkdir -p dependencies
    cd dependencies
    git clone https://github.com/cliffordwolf/icestorm.git icestorm
    cd icestorm
    git checkout 2bc541743ada3542c6da36a50e66303b9cbd2059
    NPROC=$(  grep -i "^processor" /proc/cpuinfo | wc -l )
    make -j${NPROC} 
    sudo make install

**yosys**

From [Yosys
Setup](https://github.com/YosysHQ/yosys#user-content-setup "GitHub - YosysHQ/yosys: Yosys Open SYnthesis Suite"):

    mkdir -p dependencies
    cd dependencies
    git clone https://github.com/YosysHQ/yosys
    cd yosys
    git checkout tags/yosys-0.18
    NPROC=$(  grep -i "^processor" /proc/cpuinfo | wc -l )
    make -j${NPROC}
    sudo make install

**nextpnr**

From nextpnr/README.md :

    mkdir -p dependencies
    cd dependencies
    git clone https://github.com/YosysHQ/nextpnr
    cd nextpnr
    git checkout 8d063d38b148b1e7095a032ffc9cf957c2407f32
    cmake . -DARCH=ice40
    NPROC=$(  grep -i "^processor" /proc/cpuinfo | wc -l )
    make -j${NPROC}
    sudo make install

**Tests**

*Test litex*

    python3 -m litex.soc.integration.soc_core && echo SUCCESS

Bad:

    .../python3: No module named litex.soc.integration.soc_core

Good:

    SUCCESS

*Test migen*

    echo "from migen import *" | python3 && echo "MIGEN SUCCESS"

Bad:

    Traceback (most recent call last):
      File "<stdin>", line 1, in <module>
    ImportError: No module named migen

Good:

    MIGEN SUCCESS

*Test yosys*

    echo | yosys && echo "YOSYS SUCCESS"

Bad:

    yosys: command not found

Good:

    (many lines)
    YOSYS SUCCESS

*Test icestorm*

    icebox_diff

Bad:

    icebox_diff: command not found

Good:

    Usage: icebox_diff bitmap1.asc bitmap2.asc

*Test nextpnr*

    nextpnr-ice40

Bad:

    nextpnr-ice40: command not found

Good:

    nextpnr-ice40 -- Next Generation Place and Route (...)
    (many lines)

*GCC Toolchain*

    riscv64-unknown-elf-addr2line

Bad:

    riscv64-unknown-elf-addr2line: command not found

Good:

    riscv64-unknown-elf-addr2line: « a.out »: pas de tel fichier

### <a name='adding-your-user-to-dialout-group'></a>Adding your user to Dialout group

    sudo adduser $USER dialout

Reboot the PC.

### <a name='compile-gateware-and-pogobios-(has-to-be-done-once)'></a>Compile gateware and pogobios (Has to be done once)

You have to go in the folder 'Software'

    cd Software

The following command recompile the gateware, compile the dependencies and the pogobios. (Has to be done once)

    ./pogosoc.py --target=pogobotv3 --cpu-variant=lite --build

The next command uploads the new gateware and flash the pogobios as an application. (Has to be done once)

    ./litex_term.py --serial-boot --images images.json --safe /dev/ttyUSBX

    You have to change X for the correct number 

If you want or need to change the bootloader, you need to execute the folowing commands.<br>
:warning: These commands can break your pogobot

    ./pogosoc.py --target=pogobotv3 --cpu-variant=lite --build --bootloader
    ./litex_term.py --serial-boot --images images.json --safe /dev/ttyUSBX

    You have to change X for the correct number 

### <a name='compile-the-sdk-(has-to-be-done-once-too)'></a>Compile the SDK (Has to be done once too)

It is possible to extract only a software sdk that will work with the gateware from the user space already in the robot.
The software compile with a lot of file produice with the gateware so the SDK is gateware version dependant.<br>
:warning: If no gateware is available, the robot will not run the code successfuly.<br>
:warning: If the gateware is not compatible, the robot can have some random behavior.<br>

    cd sdk
    make

All the files needed are available in the folder 'build_sdk' and can be moved where is needed.

### <a name='compile-and-test-"helloworld"'></a>Compile and test "HelloWorld"

:warning: All the examples use the SDK in order to compile. Don't forget to compile the SDK before. <br>
The error below is typical of this oversight.

    Makefile:9: ../../sdk/build_sdk/tools/common.mak: Aucun fichier ou dossier de ce type
    make: *** Aucune règle pour fabriquer la cible « ../../sdk/build_sdk/tools/common.mak ». Arrêt.

In order to test and duplicate code, you can explore the 'example' folder.

    cd pogobot/Software/example/helloworld
    make clean all

To connect using the debugging cable 

    make connect TTY=/dev/ttyUSBX

    You have to change X for the correct number 
 
    Tape "Enter" to access the prompt
    The "serialboot" command uploads and runs the application code inside the flash of the robot.
    The "run" command start the last uploaded application.

    To exit, you have to press  Crtl + C   Ctrl + C

### <a name='compile-and-upload-your-application'></a>Compile and upload your application 

The easiest way to create a new application is to duplicate "helloworld" where is pleased you.<br>
Do not forget to change the PATH to the builded SDK inside the Makefile.

### <a name='upload-a-empty-or-faulty-robot'></a>Upload a empty or faulty robot

Connection with our custom board :

|      |  C0| C1 |      |
|-----:|----|---:|------|
|    X | -o | o- | X    |
| MISO | -o | o- | MOSI |
|  CLK | -o | o- | CS   |
|  GND | -o | o- | X    |
|    X | -o | o- | X    |
|  RST | -o | o- | X    |

Prog SPI flash with FTDI (lattice ice40 Ultraplus Breakout board) :

- Connect the Lattice board.
- Connect GND between both board.
- Connect CS=>SS, SCK=>CLK, FLASH MISO=>MSIO, FLASH MOSI=>MOSI.
- Connect RST=>GND or press the RESET button on top during upload.
- Connect the DEBUG USB (White board) only for the power

<img src="Images/lattice_board_connection.jpg" alt="full connect" width="800"/>

Execute :

    iceprog build/bootloader_pogobotv3/bootloader.bin

:warning: If iceprog can't find the board but dsmesg show that everything is ok.  
"Can't find iCE FTDI USB device (vendor_id 0x0403, device_id 0x6010 or 0x6014)"

Create a file /etc/udev/rules.d/53-lattice-ftdi.rules with the following line in it to allow uploading bit-streams to a Lattice iCEstick and/or a Lattice iCE40-HX8K Breakout Board as unprivileged user:

    ATTRS{idVendor}=="0403", ATTRS{idProduct}=="6010", MODE="0660", GROUP="plugdev", TAG+="uaccess"

Prog SPI flash directly with bus-pirate :

    ../flashrom/flashrom --p buspirate_spi:dev=/dev/ttyUSB0 -w build/pogobotv3/gateware/pogobotv3_flashrom.bin 

### <a name='memory-map'></a>Memory map

This table shows addresses in flash memory. The flash memory itself is mapped in the system at address 0x200000 (SPIFLASH_BASE).


| Address in flash | Size                             | Zone size | Content          |
|------------------|----------------------------------|-----------|------------------|
| 0                | 0xa0                             | 0xa0      | Multiboot header |
| 0xa0             | 0x1969a                          | 0x20000   | Gateware         |
| 0x20000          | Size of pogobios (usually <64KB) | 0x20000   | Pogobios         |
| 0x40000          | 0x1969a                          | 0x20000   | User gateware    |
| 0x60000          | Size of user code                | 0x20000   | User software    |


### <a name='install-on-linux-distributions-other-than-ubuntu-by-using-singularity-containers'></a>Install on Linux distributions other than Ubuntu by using Singularity Containers

It is possible to build a [Singularity](https://docs.sylabs.io/guides/3.0/user-guide/index.html)/[Apptainer](https://apptainer.org/) image based on Ubuntu 20.04, which could then be used on any Linux distribution.
It may also be used to compile and install the sdk (including dependencies) if you have difficulties following the normal install procedure.

First, you need to install Singularity.
On Debian/Ubuntu distributions:
```bash
sudo apt update
sudo apt install -y wget lsb-release
export DISTRIB_NAME=$(lsb_release -c | cut -f 2)
wget https://github.com/sylabs/singularity/releases/download/v3.10.4/singularity-ce_3.10.4-${DISTRIB_NAME}_amd64.deb 
# Alternatively, download one of the .deb package available at https://github.com/sylabs/singularity/releases
sudo apt install -y ./singularity-ce_3.10.4-${DISTRIB_NAME}_amd64.deb
```
On other distributions, follow the procedure from [the official documentation](https://docs.sylabs.io/guides/3.0/user-guide/installation.html).

After that, you will need to either download an already compiled pogobot.sif image, or build it yourself.
To download an already compiled image:
```bash
cd pogobot
singularity pull --arch amd64 pogobot.sif library://leo.cazenille/pogobot/pogobot:latest
```
*Alternative*: to build it yourself (can take some time):
```bash
cd pogobot
sudo singularity build -F pogobot.sif pogobot.def
```
This will create a "pogobot.sif" image file (size: around 2GB).

Note that the singularity image contains all required applications to use the SDK -- however you'll still need to install SDK dependencies on your local computer, and compile the SDK. In order to do that, use the following commands:
```bash
cd pogobot
singularity run --app install_dep pogobot.sif
singularity run --app compile_sdk pogobot.sif
```

You can then test if the compilation of the SDK was successful:
```bash
singularity run --app test_install pogobot.sif
```

To compile the helloworld example:
```bash
cd Software/example/helloworld
singularity run --app make ../../../../pogobot.sif
```
or:
```bash
cd Software/example/helloworld
singularity exec ../../../../pogobot.sif make clean all
```

## <a name='tools'></a>Tools

### <a name='version_creation_litex.sh'></a>version_creation_litex.sh
This script is used to identify the state of all folders from litex.<br>
You have to execute the script inside the 'litex' folder to generate a file.

### <a name='doc-generation'></a>Doc generation

To generate the docs, we used a opensource project that transform .h to .md. <br>
(Requires node.js and npm to be installed on computer.)

    git https://github.com/ah01/h2md
    npm install
    cd h2md
    git checkout 21a7918a888084c20a27a9a0a8645a4e988d1bb5

Copy the custom pattern "pogobot/tools/pogobot.js" inside h2md/lib/patterns/

Add a return carrier to be understand by the tools
    
    awk '{printf "%s\r\n", $0}' /path/to/pogobot.h > pogobot.crlf

Generate docs using our own pattern

    node h2md.js pogobot.crlf -o /path/to/pogodocs.md -p pogobot -l

Remove tempory file

    rm  pogobot.crlf

## <a name='kwown-issues'></a>Kwown Issues
This section descibes some isolated issues and the woraround used.

### <a name='yosys-abc'></a>yosys-abc
During the compiling of the gateware.
```
./pogosoc.py --cpu-variant=lite --build

ERROR: ABC: execution of command "/usr/local/bin/yosys-abc -s -f /tmp/yosys-abc-DDD80t/abc.script 2>&1" failed: return code 127. [...] OSError: Error occured during Yosys/Nextpnr's script execution.
```
It seems that yosys-abc is missing from the PATH but it is here.
libreadline can have the wrong version. 
You can do this workaround (to adapt in your case).
```
sudo ln -s /lib/x86_64-linux-gnu/libreadline.so.8 /lib/x86_64-linux-gnu/libreadline.so.7
```
### <a name='nextpnr'></a>nextpnr
During the installation of nextpnr, the line "cmake . -DARCH=ice40" can block.
You can do the following workaround.

Comments the line "cmake policy(SET CMP0079 NEW)" in the file "pogobot/dependencies/nextpnr/CMakeLists.txt". 
(This line is at the begining of the file)




