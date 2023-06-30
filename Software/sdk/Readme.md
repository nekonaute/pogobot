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
- [Install on Linux distributions other than Ubuntu by using Singularity Containers](#install-on-linux-distributions-other-than-ubuntu-by-using-singularity-containers)



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


Install on Linux distributions other than Ubuntu by using Singularity Containers
================================================================================

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

After that, you will need to either download an already compiled pogobot-sdk.sif image, or build it yourself.
To download an already compiled image:
```bash
cd pogobot-sdk
singularity pull --arch amd64 pogobot-sdk.sif library://leo.cazenille/pogobot/pogobot-sdk:latest
```
*Alternative*: to build it yourself (can take some time):
```bash
cd pogobot-sdk
sudo singularity build -F pogobot-sdk.sif pogobot-sdk.def
```
This will create a "pogobot-sdk.sif" image file (size: around 1.5GB).

To compile the helloworld example:
```bash
cd example/helloworld
singularity run --app make ../../pogobot-sdk.sif
```
or:
```bash
cd example/helloworld
singularity exec ../../pogobot-sdk.sif make clean all
```
