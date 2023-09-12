#!/bin/bash

echo "using iceprog to program the robot"



iceprog -o 0x0 bootloader/bootloader_pogobotv3/bootloader.bin
iceprog -o 0x240000 pogobios/pogobotv3/gateware/pogobotv3.bin
iceprog -o 0x260000 pogobios/pogobotv3/software/pogobios/pogobios.bin
