#!/bin/bash

echo "using iceprog to program the remote"

echo "FlashIsOK" > Flash.bin

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

check_return () {
    if [[ "$1" = 0 ]]; then
        echo -e " ${GREEN}-> OK${NC}"
    else
        echo -e " ${RED}-> NOK${NC}"
        exit
    fi
}

iceprog -o 0x0 bootloader/bootloader_pogobotv3/bootloader.bin
check_return $?

iceprog -o 0x240000 remocon/remocon_pogobotv3/gateware/remocon_pogobotv3.bin
check_return $?

iceprog -o 0x260000 remocon/remocon_pogobotv3/software/pogobios/pogobios.bin
check_return $?

iceprog -o 0x88000 Flash.bin
check_return $?


rm -f Flash.bin
