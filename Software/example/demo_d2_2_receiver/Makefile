BUILD_DIR=./build
POGO_SDK=../../sdk/build_sdk

POGO_SDK_TOOLS=$(POGO_SDK)/tools
POGO_SDK_INCS=$(POGO_SDK)/includes
POGO_SDK_LIBS=$(POGO_SDK)/libs

POGO_VAR=../../build/pogobotv3/software/include/generated

include $(POGO_VAR)/variables.mak
include $(POGO_SDK_TOOLS)/common.mak


TTY?=/dev/ttyUSB0

SRCS=$(wildcard *.c)
OBJECTS=$(SRCS:.c=.o)
OBJECTS_BUILD = $(addprefix $(BUILD_DIR)/, $(OBJECTS))


all: directories $(BUILD_DIR)/firmware.bin

INCLUDES+=-I. -I$(POGO_SDK_INCS)

# pull in dependency info for *existing* .o files
-include $(OBJECTS_BUILD:.o=.d)

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf
	$(OBJCOPY) -O binary $< $@
	chmod -x $@

$(BUILD_DIR)/firmware.elf: $(OBJECTS_BUILD)
	$(CC) $(LDFLAGS) \
		-T $(POGO_SDK_TOOLS)/linker.ld \
		-N -o $@ \
		$(OBJECTS_BUILD) \
		-Wl,--whole-archive \
		-Wl,--gc-sections \
		-L$(POGO_SDK_LIBS) -lcompiler_rt -lc -lpogobot
	chmod -x $@


$(BUILD_DIR)/%.o: %.c
	$(compile)

clean:
	$(RM) -r $(BUILD_DIR) .*~ *~

connect:
	$(POGO_SDK_TOOLS)/litex_term.py --serial-boot --kernel $(BUILD_DIR)/firmware.bin --kernel-adr $(ROM_BASE) --safe $(TTY)

directories: $(BUILD_DIR)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

.PHONY: all clean connect

