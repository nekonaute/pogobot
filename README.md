# Pogobot

## Table of contents 

- [Pogobot](#pogobot)
  - [Table of contents](#table-of-contents)
  - [Overview](#overview)
    - [Git description](#git-description)
    - [Software API](#software-api)
    - [Hardware schematic](#hardware-schematic)
  - [Hardware](#hardware)
    - [Motor information](#motor-information)
    - [3d printed parts](#3d-printed-parts)
      - [Printing info](#printing-info)
      - [Assembly](#assembly)
    - [Programming the robot (hardware part)](#programming-the-robot-hardware-part)
  - [Software](#software)
    - [Repository clone](#repository-clone)
    - [Install all the dependency](#install-all-the-dependency)
    - [Compile gateware and pogobios (Has to be done ones)](#compile-gateware-and-pogobios-has-to-be-done-ones)
    - [Compile the SDK (Has to be done ones too :smile:)](#compile-the-sdk-has-to-be-done-ones-too-smile)
    - [Compile and test "HelloWorld"](#compile-and-test-helloworld)
    - [Compile and upload your application](#compile-and-upload-your-application)
    - [Upload a empty or faulty robot [WIP]](#upload-a-empty-or-faulty-robot-wip)
  - [Tools](#tools)
    - [version_creation_litex.sh](#version_creation_litexsh)
    - [Doc generation](#doc-generation)
  - [Kwown Issues](#kwown-issues)
    - [yosys-abc](#yosys-abc)
    - [nextpnr](#nextpnr)



## Overview 
The pogobot is a little robot design to evoluate in swarn. 
The robot is composed of a head (above) with a iCE40UP5K FPGA chip including a softcore processor, IMU, IR communication and a LED.
The head is plugged on a belly (under) composed of more LEDs, motor controler, battery and battery regulation.

### Git description
This project is divided into 3 folders :
- Hardware
- Software
- Tools

Hardware contains the schematics and other files to reproduice the robot. 
All the schematics were done using KiCad 6. You can find : 
- 'Belly' folder with the belly schematics
- 'Head' folder with the head schematics
- '3d-addons' folder contains all the 3D parts to print to complete a robot
- '3Dmodels' and other folders regroups all the libraries, 3D models and foorprints for KiCad.
   
Software contains the gateware / software / SDK builder to upload inside the robot.
It exists 2 softwares inside the robot. 
- The bootloader is the one called when you boot the robot. It contains a gateware and the pogobios.
- The user space is called by a command inside the bootloader. It contains your code and a gateware.
  
Tools corresponds to the differents tools used around the robot.

### Software API
The robot API is described inside [pogodocs.md](pogodocs.md). <br> 
Most of the information is contained inside pogolib/pogobot.h but if you want to go deeper, the best is to browse pogolib and pogobios. 

### Hardware schematic
Here you can find the global view of the hardware.

<img src="Images/pogobot_diagram.jpg" alt="diagram" width="800"/>

## Hardware
In order to complete a robot, you will need : 
- a belly and a head
- a 3v CR2 rechargeable Batery (Li-FePO4)
- 3 motors
- the different 3d printed parts

### Motor information
All the models are designed for the motors available here:
https://www.aliexpress.com/item/32918768766.html

<img src="Images/motors.jpg" alt="motor" width="400"/>

### 3d printed parts
All the parts are available inside the folder 'Hardware/3d-addons'.

#### Printing info
The different models have been printed in PLA with an Ultimaker2+ with a layer height of 0.2mm without support or brim.

To assemble a robot, you need to print a skirt and a capsule for the version of your belly. <br>
After printed the capsule, you have to set free the tabs by gently cut under.

<img src="Images/cut_tab.jpg" alt="cut_tab" width="400"/>

#### Assembly
- First, place the motors inside the 3 holes.
- Then, place the robot inside the capsule.
- Finally, place the robot+capsule inside the skirt.

<img src="Images/assembly.jpg" alt="assembly" width="400"/>

### Programming the robot (hardware part)
In order to program the robot, you need a USB to UART device connected between your computor and the FFC/FPC connector on the head. 
The folder 'Hardware/usb-uart-progboard' contains the Kicad files to produce a small board to program the robot. 
You also need :
- UMFT234XF board (https://www.mouser.fr/ProductDetail/FTDI/UMFT234XF?qs=NVftvxkm2thPJLzIkoAhbA%3D%3D)
- a FFC/FPC connector (8 pins, 0.5 mm, SMD/SMT, Right Angle, Dual Contact) like https://www.mouser.fr/ProductDetail/571-2328702-8
- a FFC/FPC cable (8 pins, 0.5 mm) like https://www.mouser.fr/ProductDetail/538-15166-0087
- a USB micro => USB cable

## Software

The Software is divided between the code for the FPGA (gateware) and the code for the softcore (software). 

:warning: Everything was tested with Ubuntu 20.04

### Repository clone
    git clone https://github.com/nekonaute/pogobot.git
    cd pogobot


### Install all the dependency

Create dependencies directory

    mkdir -p dependencies

**system dependency**

The list is adapted for Ubuntu distribution

    sudo apt-get -y install bison build-essential clang clang-format cmake flex gawk git graphviz libboost-all-dev libboost-dev libboost-filesystem-dev libboost-iostreams-dev libboost-program-options-dev libboost-python-dev libboost-system-dev libboost-thread-dev libeigen3-dev libffi-dev libftdi-dev libreadline-dev mercurial pkg-config python python3 python3-dev python3-pip python3-setuptools qt5-default tcl-dev xdot zlib1g-dev

    pip3 install meson ninja
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
    ./litex_setup.py --gcc riscv

Adding compiler in the path (replace * with the correct name)

    export GCC_RISCV=$PWD/riscv64-*/bin/
    echo "export PATH=\"\$PATH:$GCC_RISCV\"" >>~/.bashrc


Notice the option `update` that updates a litex but it can break the project.

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

    riscv64-unknown-elf-addr2line: ????a.out????: pas de tel fichier


### Compile gateware and pogobios (Has to be done ones)

You have to go in the folder 'Software'

    cd Software

The following command recompile the gateware, compile the dependencies and the pogobios. (Has to be done ones)

    ./pogosoc.py --target=pogobotv2_1 --cpu-variant=lite --build

The next command uploads the new gateware and flash the pogobios as an application. (Has to be done ones)

    ./litex_term.py --serial-boot --images images.json --safe /dev/ttyUSBX

    You have to change X for the correct number 

If you want or need to change the bootloader, you need to execute the folowing commands.<br>
:warning: These commands can break your pogobot

    ./pogosoc.py --target=pogobotv2_1 --cpu-variant=lite --build --bootloader
    ./litex_term.py --serial-boot --images images.json --safe /dev/ttyUSBX

    You have to change X for the correct number 

### Compile the SDK (Has to be done ones too :smile:)

It is possible to extract only a software sdk that will work with the gateware from the user space already in the robot.
The software compile with a lot of file produice with the gateware so the SDK is gateware version dependant.<br>
:warning: If no gateware is available, the robot will not run the code successfuly.<br>
:warning: If the gateware is not compatible, the robot can have some random behavior.<br>

    cd sdk
    make

All the files needed are available in the folder 'build_sdk' and can be moved where is needed.

### Compile and test "HelloWorld"

:warning: All the examples use the SDK in order to compile. Don't forget to compile the SDK before. <br>
The error below is typical of this oversight.

    Makefile:9: ../../sdk/build_sdk/tools/common.mak: Aucun fichier ou dossier de ce type
    make: *** Aucune r??gle pour fabriquer la cible ????../../sdk/build_sdk/tools/common.mak????. Arr??t.

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

### Compile and upload your application 

The easiest way to create a new application is to duplicate "helloworld" where is pleased you.<br>
Do not forget to change the PATH to the builded SDK inside the Makefile.

### Upload a empty or faulty robot [WIP]

TODO :

- list connection
- adress 0x0 no Ox40000 to verify

Prog SPI flash directly with bus-pirate :

    ../flashrom/flashrom --p buspirate_spi:dev=/dev/ttyUSB0 -w build/pogobotv2/gateware/pogobotv2_flashrom.bin 

Prog SPI flash with FTDI (from Breakout board) :

    iceprog -o 0x40000 build/pogobotv2/image.bin

## Tools

### version_creation_litex.sh
This script is used to identify the state of all folders from litex.<br>
You have to execute the script inside the 'litex' folder to generate a file.

### Doc generation

To generate the docs, we used a opensource project that transform .h to .md. <br>
(Requires node.js and npm to be installed on computer.)

    git https://github.com/ah01/h2md
    npm install
    cd h2md

Copy the custom pattern pogobot/tools/pogobot.js inside h2md/lib/patterns/

Add a return carrier to be understand by the tools
    
    awk '{printf "%s\r\n", $0}' /path/to/pogobot.h > pogobot.crlf

Generate docs using our own pattern

    node h2md.js pogobot.crlf -o /path/to/pogodocs.md -p pogobot -l

Remove tempory file

    rm  pogobot.crlf


## Kwown Issues
This section descibes some isolated issues and the woraround used.

### yosys-abc
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
### nextpnr
During the installation of nextpnr, the line "cmake . -DARCH=ice40" can block.
You can do the following workaround.

Comments the line "cmake policy(SET CMP0079 NEW)" in the file "pogobot/dependencies/nextpnr/CMakeLists.txt". 
(This line is at the begining of the file)




