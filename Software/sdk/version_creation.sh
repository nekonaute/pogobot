#/bin/bash
LANG=en_us_8859_1 

# usage : 
# ./version_creation ../toto

if [ -z "$1" ]
then
    DEST=version.h
    echo "default PATH used :" ${DEST}
else
    DEST=$1/version.h
    echo "PATH used : " $1
fi

DEPEN_DIR=../../dependencies

if [ -d ${DEPEN_DIR} ]
then
    echo ${DEPEN_DIR} "exists. OK"
else
    echo "Error: Directory "${DEPEN_DIR}" does not exists."
    echo "Please change the variable DEPEN_DIR to the correct dependencies location" 
    exit 1
fi


echo "//auto generated (" `date` ")" > ${DEST}
echo "#ifndef POGO_VERSION" >> ${DEST}
echo "#define POGO_VERSION\n\n" >> ${DEST}

echo "#define POGOBOT_VERSION " `git log | head -n 1 | sed 's/commit //'` >> ${DEST}
echo "#define NEXTPNR_VERSION " `git -C ${DEPEN_DIR}/nextpnr/ log | head -n 1 | sed 's/commit //'` >> ${DEST}
echo "#define LITEX_VERSION " `git -C ${DEPEN_DIR}/litex/litex/ log | head -n 1 | sed 's/commit //'` >> ${DEST}
echo "#define MIGEN_VERSION " `git -C ${DEPEN_DIR}/litex/migen/ log | head -n 1 | sed 's/commit //'` >> ${DEST}
echo "#define YOSYS_VERSION " `git -C ${DEPEN_DIR}/yosys log | head -n 1 | sed 's/commit //'` >> ${DEST}
echo "#define ICESTORM_VERSION " `git -C ${DEPEN_DIR}/icestorm log | head -n 1 | sed 's/commit //'` >> ${DEST}

echo "\n" >> ${DEST}

MYVAR=`riscv64-unknown-elf-gcc --version | head -n 1`
echo "#define GCC_VERSION " ${MYVAR##* } >> ${DEST}

echo "\n\n#endif\n" >> ${DEST}

