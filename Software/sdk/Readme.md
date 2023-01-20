Pogobot
=======

Table of contents
-----------------

- [Pogobot](#pogobot)
  - [Table of contents](#table-of-contents)
- [Overview](#overview)
- [Requirements](#requirements)
- [Install the dependencies](#install-the-dependencies)
  - [System dependency](#system-dependency)
  - [Litex](#litex)
  - [Cross Compiler](#cross-compiler)
  - [Adding your user to Dialout group](#adding-your-user-to-dialout-group)
- [pogoSDK](#pogosdk)
  - [Compile examples](#compile-examples)
  - [Compile and upload your own application](#compile-and-upload-your-own-application)



Overview
========

We designed a robot with motors, SoC (system-on-chip) inside a
iCE40LP8K FPGA chip including a softcore processor, motors, sensors, LED, IR communication.
This SDK allows you to compile and upload code on the robot.

Please visit : https://github.com/nekonaute/pogobot-sdk or https://github.com/nekonaute/pogobot

Requirements
============

The SDK works only on Ubuntu and is tested with Ubuntu 20.04.3 LTS and riscv64 gnu tools 8.3.0 and 10.1.0.


Install the dependencies
========================

Create dependencies directory

    mkdir -p dependencies

System dependency
-----------------

The list is adapted for Ubuntu distribution

    sudo apt-get -y install wget bison build-essential clang clang-format cmake flex gawk git graphviz libboost-all-dev libboost-dev libboost-filesystem-dev libboost-iostreams-dev libboost-program-options-dev libboost-python-dev libboost-system-dev libboost-thread-dev libeigen3-dev libffi-dev libftdi-dev libreadline-dev mercurial pkg-config python python3 python3-dev python3-pip python3-setuptools qt5-default tcl-dev xdot zlib1g-dev

    sudo pip3 install meson ninja
    echo "export PATH=\"\$PATH:~/.local/bin\"" >>~/.bashrc

Litex
-----

From https://github.com/enjoy-digital/litex

    mkdir -p dependencies/litex
    cd dependencies/litex
    wget https://raw.githubusercontent.com/enjoy-digital/litex/master/litex_setup.py
    chmod +x litex_setup.py
    ./litex_setup.py init

    cat ../../tools/litex_version.txt | awk '{ if(!system("test -d " $1)) {system("git -C " $1 " checkout " $3)}}'

    sudo ./litex_setup.py install


Cross Compiler
--------------

You need the cross-compiler gcc for the softcore.

    cd tools
    wget https://static.dev.sifive.com/dev-tools/freedom-tools/v2020.08/riscv64-unknown-elf-gcc-10.1.0-2020.08.2-x86_64-linux-ubuntu14.tar.gz
    tar xvf riscv64-unknown-elf-gcc-10.1.0-2020.08.2-x86_64-linux-ubuntu14.tar.gz

Adding compiler in the path

    export GCC_RISCV=$PWD/riscv64-unknown-elf-gcc-10.1.0-2020.08.2-x86_64-linux-ubuntu14/bin/
    echo "export PATH=\"\$PATH:$GCC_RISCV\"" >>~/.bashrc

Adding your user to Dialout group
---------------------------------

    sudo adduser $USER dialout

Reboot the PC.

pogoSDK
=======

Compile examples
----------------

The SDK comes with somes examples.
To compile and upload one. Please go to the example folder you want to test.

    make all
    make connect TTY=/dev/ttyUSB0

Inside the robot prompt, type "enter" to obtain a new prompt line.
After type the command "serialboot" to upload the code. 


Compile and upload your own application
---------------------------------------

To make a new application, duplicate an example. 
If you move to a new folder, don't forget to change the SDK path inside the Makefile.

