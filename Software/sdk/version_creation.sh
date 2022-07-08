#/bin/bash

# ./version_creation ../toto

if [ -z "$1" ]
then
    echo "default PATH used"
    DEST=version.h
else
    DEST=$1/version.h
    echo "PATH used : " $1
fi



echo "#ifndef POGO_VERSION" > ${DEST}
echo "#define POGO_VERSION\n\n" >> ${DEST}


echo "#define POGOBOT_VERSION " `git log | head -n 1 | sed 's/commit //'` >> ${DEST}
echo "#define NEXTPNR_VERSION " `git -C ../../nextpnr/ log | head -n 1 | sed 's/commit //'` >> ${DEST}
echo "#define LITEX_VERSION " `git -C ../../litex/litex/ log | head -n 1 | sed 's/commit //'` >> ${DEST}
echo "#define MIGEN_VERSION " `git -C ../../litex/migen/ log | head -n 1 | sed 's/commit //'` >> ${DEST}
echo "#define YOSYS_VERSION " `git -C ../../yosys log | head -n 1 | sed 's/commit //'` >> ${DEST}
echo "#define ICESTORM_VERSION " `git -C ../../icestorm log | head -n 1 | sed 's/commit //'` >> ${DEST}

echo "\n" >> ${DEST}

MYVAR=`riscv64-unknown-elf-gcc --version | head -n 1`
echo "#define GCC_VERSION " ${MYVAR##* } >> ${DEST}

echo "\n\n#endif\n" >> ${DEST}

