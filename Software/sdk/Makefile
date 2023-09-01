TARGET ?=pogobotv3
BUILD_POGO_DIR=../build/$(TARGET)
POGOLIB_BUILDDIR=$(BUILD_POGO_DIR)/software/pogolib

BUILD_SDK_DIR=./build_sdk

include $(BUILD_POGO_DIR)/software/include/generated/variables.mak

ifeq "${SOC_DIRECTORY}" ""
    $(error It seems that the TARGET is wrong or the gateware for '${TARGET}' is not already compiled)
endif

include $(SOC_DIRECTORY)/software/common.mak

POGOBIOS_DIR=../pogobios
POGOLIB_DIR=../pogolib

all: directories sdk

$(POGOLIB_BUILDDIR)/libpogobot.a: 
	make -C $(POGOLIB_DIR) TARGET=$(TARGET)

directories: $(BUILD_SDK_DIR)

$(BUILD_SDK_DIR):
	mkdir -p $(BUILD_SDK_DIR)/tools
	mkdir -p $(BUILD_SDK_DIR)/includes
	mkdir -p $(BUILD_SDK_DIR)/libs
	mkdir -p $(BUILD_SDK_DIR)/tools/include/litex/soc
	mkdir -p $(BUILD_SDK_DIR)/tools/include/litex/pythondata-software-picolibc/pythondata_software_picolibc/data
	mkdir -p $(BUILD_SDK_DIR)/tools/include/pogobotv3/software/include
	mkdir -p $(BUILD_SDK_DIR)/tools/include/litex/soc/software/libbase


sdk: $(POGOLIB_BUILDDIR)/libpogobot.a
	echo $(PWD)
	cp variables.mak $(BUILD_SDK_DIR)/tools
	cp ../../litex_version.txt $(BUILD_SDK_DIR)/tools/
	cp $(SOC_DIRECTORY)/software/common.mak $(BUILD_SDK_DIR)/tools*
	cp -r $(SOC_DIRECTORY)/* $(BUILD_SDK_DIR)/tools/include/litex/soc
	cp -r $(PICOLIBC_DIRECTORY)/* $(BUILD_SDK_DIR)/tools/include/litex/pythondata-software-picolibc/pythondata_software_picolibc/data
	cp -r $(BUILDINC_DIRECTORY)/* $(BUILD_SDK_DIR)/tools/include/pogobotv3/software/include
	cp -r $(BUILDINC_DIRECTORY)/../libc $(BUILD_SDK_DIR)/tools/include/pogobotv3/software
	cp -r $(LIBBASE_DIRECTORY)/* $(BUILD_SDK_DIR)/tools/include/litex/soc/software/libbase
	find $(BUILD_SDK_DIR)/tools/include/ -type f ! \( -name "*.h" -or -name "*.ld" \) -delete
	find $(BUILD_SDK_DIR)/tools/include/ -type d -empty -delete
	find $(SOC_DIRECTORY)/cores/cpu/* -type d ! -name "vexriscv" -exec rm -r {} +
	cp $(POGOLIB_DIR)/*.h $(BUILD_SDK_DIR)/includes
	cp $(POGOBIOS_DIR)/linker.ld $(BUILD_SDK_DIR)/tools
	cp ../litex_term.py $(BUILD_SDK_DIR)/tools
	cp $(POGOLIB_BUILDDIR)/libpogobot.a $(BUILD_SDK_DIR)/libs
	cp $(BUILD_POGO_DIR)/software/libcompiler_rt/libcompiler_rt.a $(BUILD_SDK_DIR)/libs
	cp $(BUILD_POGO_DIR)/software/libc/libc.a $(BUILD_SDK_DIR)/libs
	cp -r ../example $(BUILD_SDK_DIR)/examples
	find $(BUILD_SDK_DIR)/examples -maxdepth 1 -mindepth 1 -type d -exec sed -i 's/POGO_SDK=..\/..\/sdk\/build_sdk/POGO_SDK=..\/../g' {}/Makefile \;
	find $(BUILD_SDK_DIR)/examples -maxdepth 1 -mindepth 1 -type d -exec sed -i 's/POGO_VAR=..*/POGO_VAR=$$(POGO_SDK_TOOLS)/g' {}/Makefile \;
	make -C $(BUILD_SDK_DIR)/examples/ clean
	./version_creation.sh ./build_sdk/includes
	cp Readme.md $(BUILD_SDK_DIR)
	cp ../../pogodocs.md  $(BUILD_SDK_DIR)



clean:
	$(RM) -r $(BUILD_SDK_DIR)

.PHONY: all clean
