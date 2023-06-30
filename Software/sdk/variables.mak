CURRENT_DIRECTORY := $(dir $(lastword $(MAKEFILE_LIST)))

TRIPLE=riscv64-unknown-elf
CPU=vexriscv
CPUFAMILY=riscv
CPUFLAGS=-march=rv32im     -mabi=ilp32 -D__vexriscv__
CPUENDIANNESS=little
CLANG=0
CPU_DIRECTORY=${CURRENT_DIRECTORY}/include/litex/soc/cores/cpu/vexriscv
SOC_DIRECTORY=${CURRENT_DIRECTORY}/include/litex/soc
PICOLIBC_DIRECTORY=${CURRENT_DIRECTORY}/include/litex/pythondata-software-picolibc/pythondata_software_picolibc/data
BUILDINC_DIRECTORY=${CURRENT_DIRECTORY}/include/pogobotv3/software/include
LIBBASE_DIRECTORY=${CURRENT_DIRECTORY}/include/litex/soc/software/libbase
SPIFLASH_BASE=0x200000
ROM_BASE=0x260000
