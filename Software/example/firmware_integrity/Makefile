BUILD_DIR=./build
POGO_SDK=../../sdk/build_sdk

POGO_SDK_TOOLS=$(POGO_SDK)/tools
POGO_SDK_INCS=$(POGO_SDK)/includes
POGO_SDK_LIBS=$(POGO_SDK)/libs

include $(POGO_SDK_TOOLS)/variables.mak
include $(POGO_SDK_TOOLS)/common.mak

TTY?=/dev/ttyUSB0
PAYLOAD_BYTES?=32768

all: $(BUILD_DIR)/firmware.bin

INCLUDES+=-I. -I$(POGO_SDK_INCS)

-include $(BUILD_DIR)/main.d

$(BUILD_DIR)/main.o: main.c | $(BUILD_DIR)
	$(compile)

$(BUILD_DIR)/firmware.elf: $(BUILD_DIR)/main.o
	$(CC) $(LDFLAGS) \
		-T $(POGO_SDK_TOOLS)/linker.ld \
		-N -o $@ \
		$< \
		-Wl,--whole-archive \
		-Wl,--gc-sections \
		-L$(POGO_SDK_LIBS) -lcompiler_rt -lc -lpogobot
	chmod -x $@

$(BUILD_DIR)/firmware.raw.bin: $(BUILD_DIR)/firmware.elf
	$(OBJCOPY) -O binary $< $@

# Reseal on each invocation so PAYLOAD_BYTES can change without recompilation.
$(BUILD_DIR)/firmware.bin: $(BUILD_DIR)/firmware.raw.bin seal_firmware.py FORCE
	$(PYTHON) seal_firmware.py $< $@ $(PAYLOAD_BYTES) $(BUILD_DIR)/firmware.elf $(TRIPLE)-nm
	chmod -x $@

$(BUILD_DIR):
	mkdir -p $@

connect:
	$(POGO_SDK_TOOLS)/litex_term.py --serial-boot --kernel $(BUILD_DIR)/firmware.bin --kernel-adr $(ROM_BASE) --safe $(TTY)

clean:
	$(RM) -r $(BUILD_DIR)

FORCE:

.PHONY: all connect clean FORCE
