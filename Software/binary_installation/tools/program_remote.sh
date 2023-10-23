#!/bin/bash

echo "using iceprog to program the remote"

iceprog -o 0x0 bootloader/bootloader_pogobotv3/bootloader.bin
iceprog -o 0x240000 remocon/remocon_pogobotv3/gateware/remocon_pogobotv3.bin
iceprog -o 0x260000 remocon/remocon_pogobotv3/software/pogobios/pogobios.bin
