#!/bin/bash

# needed files
#./bootloader/bootloader_pogobotv3/bootloader.bin
#./program_robot.sh
#./pogobios/pogobotv3/gateware/pogobotv3.bin
#./pogobios/pogobotv3/software/pogobios/pogobios.bin
#./program_remote.sh
#./remocon/remocon_pogobotv3/gateware/remocon_pogobotv3.bin
#./remocon/remocon_pogobotv3/software/pogobios/pogobios.bin


while getopts v:h: flag
do
    case "${flag}" in
        v) version=${OPTARG};;
        h) fhelp=${OPTARG};;    
    esac
done

if [ ! -z "$fhelp" ]
then
    echo "Write the version with the -v option (ex 2.2.1)"
    echo "    create_new_version.sh -v2.2.1"
    exit
fi

if [ -z "$version" ]
then
      echo "Version is empty"
else
      echo "Chosen version is: $version"
      INSTALL_FOLDER="install_APIv"$version 

      mkdir -p ../$INSTALL_FOLDER
      # program_remote.sh  program_robot.sh
      cp "program_remote.sh" ../$INSTALL_FOLDER
      cp "program_robot.sh" ../$INSTALL_FOLDER

      # recover binary
      mkdir -p ../$INSTALL_FOLDER/bootloader/bootloader_pogobotv3/
      mkdir -p ../$INSTALL_FOLDER/pogobios/pogobotv3/gateware/
      mkdir -p ../$INSTALL_FOLDER/pogobios/pogobotv3/software/pogobios/
      mkdir -p ../$INSTALL_FOLDER/remocon/remocon_pogobotv3/gateware/
      mkdir -p ../$INSTALL_FOLDER/remocon/remocon_pogobotv3/software/pogobios/

      cp ../../build/bootloader_pogobotv3/bootloader.bin ../$INSTALL_FOLDER/bootloader/bootloader_pogobotv3/bootloader.bin
      cp ../../build/pogobotv3/gateware/pogobotv3.bin ../$INSTALL_FOLDER/pogobios/pogobotv3/gateware/pogobotv3.bin
      cp ../../build/pogobotv3/software/pogobios/pogobios.bin ../$INSTALL_FOLDER/pogobios/pogobotv3/software/pogobios/pogobios.bin
      cp ../../build/remocon_pogobotv3/gateware/remocon_pogobotv3.bin ../$INSTALL_FOLDER/remocon/remocon_pogobotv3/gateware/remocon_pogobotv3.bin
      cp ../../build/remocon_pogobotv3/software/pogobios/pogobios.bin ../$INSTALL_FOLDER/remocon/remocon_pogobotv3/software/pogobios/pogobios.bin

      echo "Version $version create inside the folder ../$INSTALL_FOLDER"

fi


