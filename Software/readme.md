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

    sudo apt-get -y install wget bison build-essential clang clang-format cmake flex gawk git graphviz libboost-all-dev libboost-dev libboost-filesystem-dev libboost-iostreams-dev libboost-program-options-dev 
    
    sudo apt-get -y install libboost-python-dev libboost-system-dev libboost-thread-dev libeigen3-dev libffi-dev libftdi-dev libreadline-dev mercurial pkg-config python3 python3-dev python3-pip python3-setuptools qt5-default tcl-dev xdot zlib1g-dev

    sudo pip3 install meson==0.64.1 ninja
    echo "export PATH=\"\$PATH:~/.local/bin\"" >>~/.bashrc

Configure the environnement 

    export LC_ALL=LC.UTF-8
    export LANG=LC.UTF-8
    export PATH =\$PATH:/pogobot/dependencies/riscv64-unknown-elf-gcc-10.1.0-2020.08.2-x86_64-linux-ubuntu14/bin

Add user to dialout ( to get access to peripherals )
    sudo adduser $USER dialout

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

    export PATH=$HOME/.local/bin:$PATH


Notice the option `update` of litex_setup.py have a high probability to break the project.

**icestorm**

From [Icestorm installation
instructions](http://www.clifford.at/icestorm/#install):

    cd dependencies
    git clone https://github.com/cliffordwolf/icestorm.git icestorm
    cd icestorm
    git checkout 2bc541743ada3542c6da36a50e66303b9cbd2059
    NPROC=$(  grep -i "^processor" /proc/cpuinfo | wc -l )
    make -j${NPROC} 
    make install PREFIX=$HOME/.local

**yosys**

From [Yosys
Setup](https://github.com/YosysHQ/yosys#user-content-setup "GitHub - YosysHQ/yosys: Yosys Open SYnthesis Suite"):

    cd dependencies
    git clone https://github.com/YosysHQ/yosys
    cd yosys
    git checkout tags/yosys-0.18
    NPROC=$(  grep -i "^processor" /proc/cpuinfo | wc -l )
    make -j${NPROC}
    make install PREFIX=$HOME/.local

**nextpnr**

From nextpnr/README.md :

    cd dependencies
    git clone https://github.com/YosysHQ/nextpnr
    cd nextpnr
    git checkout 8d063d38b148b1e7095a032ffc9cf957c2407f32
    cmake . -DARCH=ice40 -DCMAKE_INSTALL_PREFIX=$HOME/.local
    NPROC=$(  grep -i "^processor" /proc/cpuinfo | wc -l )
    make -j${NPROC}
    make install

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
    ./litex_term.py --serial-boot --kernel=build/bootloader_pogobotv3/bootloader.bin --kernel-adr=0x200000 --safe /dev/ttyUSBX

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
| 0x80000          | Reserved; validity marker at 0x88000 | 0x10000 | Upload state, excluded from user erases |
| 0x90000          | 5888 pages of 256 bytes          | 0x170000  | User-writable data on v3, through 0x1FFFFF |


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

Generate docs using our own pattern

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

### <a name='egg-link'></a>nextpnr

During yosys installation.

Note : Egg-link : fill used by setuptools when a Python package is installed and defined as editable (dev mode). It points to the user local folder.

Suppress egg-info files using (on Ubuntu):
rm -f ~/.local/lib/python*/site-packages/*-link .
## PFFS file commands

Pogobios on v3 robots includes PFFS v4 file management at both `Pogoboot>`
and `Pogobot>`. The remote console also supports these UART commands. Run
`pffs` for filesystem help, or `help` for the complete BIOS command list.
The filesystem is never formatted automatically by booting or by these commands.
File management commands act on the UART-connected device. Connecting to a Pogoshower manages
the controller's own files; robot file transfers require the robot's UART and
are not forwarded over IR.

| Command | Action |
| --- | --- |
| `ls` | List files and validate catalog, ordinary-file and log CRCs. |
| `df` | Show allocated sectors, free capacity and file-ID usage. |
| `stat <id\|name>` | Display metadata and log committed length. |
| `cat <id\|name>` | Hex-dump an ordinary file; display a log with binary bytes escaped. |
| `load <id\|name>` | On robots, install a raw controller binary in the application slot; then use `run`. |
| `load_run <name>` | On robots, install and start a named controller; accepts UART or IR, red on failure. |
| `touch <id> <name\|-> [bytes]` | Create an ordinary file; default length is zero. |
| `write <id\|name> <offset> <hexbytes>` | Edit a one-page ordinary file, extending its length if needed. |
| `mv <id\|name> <new_name\|->` | Change the optional name without changing the ID. |
| `rm <id\|name>` | Remove the catalog entry. |
| `format YES` | Explicitly delete all PFFS files, including calibration files. |

For example, on an already formatted filesystem:

```text
touch 42 notes
write notes 0 4869
cat notes
stat 42
```

IDs are 1–80; names are case-sensitive and up to 32 bytes without spaces.
The existing 5888-page user-flash region is unchanged. PFFS reserves 64 KiB
at physical `0x90000–0x9FFFF` for catalogs and uses the following 1408 KiB
at `0xA0000–0x1FFFFF` for file extents. Raw page writes by an application can
still overwrite this filesystem; applications sharing it must use the same
PFFS v4 layout. The SDK exposes the file and log APIs through `<pogobot.h>`
(or `<pffs/flash_file.h>` and `<pffs/flash_log.h>`) and links them from
`libpogobot.a`. Pogobios uses that same implementation; applications still
link their own executable and do not call into BIOS runtime services.
Existing `pogo-utils/flash_file.h` and `flash_log.h` includes forward to the SDK.
See [library integration](pogolib/pffs/README.md) for backend and formatting policy.

Compaction is a maintenance operation available as `defrag YES` in the
`pogo-utils/examples/flash_file` application. Pogobios omits that command;
the shared library retains its defragmentation API. After deletions, a large
file may need compaction to obtain a contiguous extent even when `df` reports
enough total free space.

Close the serial terminal before using the host transfer tool, then run these
commands from `Software/`. The robot must be at its Pogobios prompt:

```sh
python3 pffs_transfer.py --port /dev/ttyUSB0 upload record.bin --id 10 --name record
python3 pffs_transfer.py --port /dev/ttyUSB0 download record received.bin
```

The same commands work with `pogo-utils/tools/pffs_transfer.py`. Raw upload
is the default and catalogs retain the exact file length. `--envelope` adds
an optional PFT1 header. Replacing an existing file requires `--replace`, a
matching ID/name/type and an unchanged allocated page count. The protocol uses
48-byte chunks with CRCs, acknowledgements and retries. A disconnected session
expires after 30 seconds, even with an unfinished console line. The BIOS
accepts up to 159 input characters and rejects an overlong command in full.

UART transfers own the filesystem until completion or timeout.
Other console commands are rejected during those transfers; IR messages are
drained without entering a firmware upload. Start or retry `rc_flash_robot`
after the file operation finishes. File management commands themselves are UART-only.
Console history and completion remain available in the full terminal build.

To start a controller already stored on neighboring robots, the remote command
`rc_load_run <name>` repeats the bounded robot boot action `load_run <name>`
every 200 ms until a key is pressed. It resolves an exact filename, verifies
and installs the controller, and reboots only on success. A missing/invalid
file or failed install shows a red LED/idle blink; it never starts an older
image as a fallback. Receiving robots must be idle at the BIOS prompt. The
remote needs no copy of the file. See the [remote guide](../readme-irRemote.md#start-a-controller-stored-in-pffs).

To keep several controllers in PFFS and select one later, first upload the
SDK-built `firmware.bin` as an ordinary raw file. From `Software/`, with the
robot at its BIOS prompt and the serial terminal closed:

```sh
python3 pffs_transfer.py --port /dev/ttyUSB0 upload path/to/firmware.bin --id 20 --name controller_a
```

Reconnect the terminal and select the controller by name or numeric ID:

```text
load controller_a
run
```

`load` behaves like a completed `serialboot`: it installs persistently at
physical `0x60000` (CPU address `0x260000`) and leaves the prompt available.
It does not reboot automatically. The saved PFFS file remains intact. Return to
`Pogoboot>` before selecting another controller: `run 0` works from a BIOS
console, or power-cycle a running application. Use raw `.bin` images linked for
that address; ELF files, PFT1 envelopes
and logs are rejected. Files must be 4–131,072 bytes and fit the existing
128 KiB application slot; this does not enlarge the SDK linker or SRAM limits.
The command is UART-only on robots and is absent from the remote console.

The loader validates the stored file before erasing the current image, then
checks programmed bytes and the whole installed CRC before committing the
validity marker. It does not erase gateware, the bootloader or PFFS. Installation
replaces the previous controller: interruption after erasure can leave a partial
image, which must be installed again from `Pogoboot>`. The saved source remains
available for that retry. Loading is synchronous and blocks other console work
until it succeeds or reports a flash error.

### Broadcast a saved file over IR

Store a named ordinary file in the sender's PFFS, then use one of these commands
at its BIOS prompt (the optional pass count defaults to one):

```text
ir_send_file controller_a 3
```

On a Pogowall/Pogoshower controller, use the remote variant:

```text
rc_send_file controller_a 3 --replace
```

The sender reads its own flash, so the PC is not involved in transmission. Both
variants broadcast through all IR emitters at full power, using 64-byte chunks,
16+4 Reed–Solomon parity and four-group interleaving. Only one transmitter should
broadcast at a time. `Q` or Escape stops it; progress describes sent chunks,
and the final message explicitly leaves receiver completion unconfirmed. There
are no acknowledgements to the sender and no automatic multi-hop relaying.

Receiving robots need this updated BIOS and an idle `Pogoboot>` or `Pogobot>`
console. A valid START enters reception automatically. The file keeps its name,
payload format and exact length; each receiver chooses a free local ID. Files
may contain 0..1,441,792 bytes (1,408 KiB). Names must be 1..32 printable ASCII
bytes without spaces. Append-log containers are unsupported; ordinary files,
including raw controllers and optional PFT1 envelopes, retain their bytes.

By default, an existing different file with the same name is refused. An already
verified identical copy is skipped. `--replace` permits overwriting that name,
preserves its ID and requires the same payload format and allocated page count.
The exact byte length may change within that allocation. A missing name is
created even with `--replace`. Replacement is in place and an interruption may
damage the old file; it is not rollback or a resizing operation.

A new file remains unpublished until all chunks, programmed bytes and the final
CRC are verified. Lost chunks can be repaired by later passes or by repeating
the same command/flags while the receiver stays powered. STARTs are bound to
sender, content CRC, length, name, format and replacement policy. Transfer IDs
are stable hashes of that metadata. Receiver logs report success and the local
ID, or an incomplete/error state. A successful file transfer leaves firmware
and boot-validity markers unchanged. Execute `load <name>` and `run` separately
to install a received controller.

An incomplete reception retains its extent and RAM bitmap after timeout or
network cancellation. Until it completes, the receiver locks other file/flash
commands to protect that unpublished allocation. After reception stops, discard
it explicitly at the receiver's UART console with:

```text
ir_file_abort
```

This releases the reservation, and does not undo replacement writes. Reboot also
loses the RAM bitmap/reservation; an unpublished new file will be received from
the beginning. A different pending file needs an abort first. No radio command
implicitly formats a missing, corrupt or older catalog.

The sender allows 500 ms per allocated 4 KiB sector plus 3 s for preparation,
with repeated START keepalives; DATA/parity pacing is 200 ms per frame. Thus
large files can take a long time. These conservative defaults and IR range,
loss recovery and stack margins still need hardware measurement.

Catalog changes and replacements are not power-loss atomic. Keep power
connected and back up important data before those operations. The maintenance
application's overlapping defrag moves have the same limitation.
`format YES` invalidates catalogs rather than securely wiping payload bytes.
Corrupt or older catalogs are rejected without automatic migration or erasure.

Host verification runs `python3 tests/run_pffs_test.py` from `Software/`.
It uses production code with strict NOR flash and UART test doubles, fatal
address/undefined-behavior sanitizers and LTO. Hardware upload timing and peak
stack use need validation on a robot.


## Build and library verification

Pogobios and pogolib enable LTO and function/data sections by default. Use
`LTO=0` in both builds after cleaning to compare without LTO. Fat objects keep
native code in `libpogobot.a` for applications that link without LTO. Changing
LTO mode requires a clean rebuild or `make -B`.

Run `python3 Software/tests/run_api_fault_test.py` to exercise production IR
send bounds and FIFO backpressure, full security-register writes, and signed
timer conversions under address/undefined-behavior sanitizers. These checks
use simulated hardware; target timing and peripherals need robot validation.


## Program and user-data flash capacity on v3

The linker allowance for normal and bootloader Pogobios, and generated SDK
programs, is 96 KiB. The combined bootloader reserves that complete allowance
at offset 0x20000. The physical firmware upload slot remains 128 KiB.

The separate user-data region spans physical 0x90000..0x1fffff: 5888 pages of
256 bytes on a v3 head with 2 MiB flash. Page APIs accept uint16_t and reject
IDs outside 0..5887. Gateware, application firmware, CRC metadata and validity
markers remain outside this region. Rebuild applications against the updated
SDK declarations. The flash-page example checks boundaries including pages
255, 256 and 5887; erasing the user region removes all data stored there.
