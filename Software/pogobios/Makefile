TARGET ?=pogobotv2
BUILD_DIR=../build/$(TARGET)
PB_BD=$(BUILD_DIR)/software/pogobios
LB_BD=$(BUILD_DIR)/software/libbase
POGOBIOS_DIR=./
POGOLIB_DIR=../pogolib

include $(BUILD_DIR)/software/include/generated/variables.mak
include $(SOC_DIRECTORY)/software/common.mak

OBJECTS = \
		$(PB_BD)/superbot_matrix.o			\
		$(PB_BD)/superbot_neural_network.o	\
		$(PB_BD)/boot.o						\
		$(PB_BD)/boot-helper.o				\
		$(PB_BD)/ir_boot.o					\
		$(PB_BD)/cmd_ann.o					\
		$(PB_BD)/cmd_bios.o					\
		$(PB_BD)/cmd_ir.o					\
		$(PB_BD)/cmd_pogobot.o				\
		$(PB_BD)/cmd_timer.o				\
		$(PB_BD)/helpers.o					\
		$(PB_BD)/cmd_mem.o					\
		$(PB_BD)/cmd_boot.o					\
		$(PB_BD)/memtest.o					\
		$(PB_BD)/crc16.o					\
		$(PB_BD)/crc32.o					\
		$(PB_BD)/system.o					\
		$(PB_BD)/spiflash.o					\
		$(PB_BD)/main.o	

ifneq "$(or $(TERM_NO_COMPLETE),$(TERM_MINI))" ""
	CFLAGS += -DTERM_NO_COMPLETE
else
	OBJECTS += $(PB_BD)/complete.o
endif

ifdef TERM_NO_HIST
	CFLAGS += -DTERM_NO_HIST
endif

ifdef TERM_MINI
	CFLAGS += -DTERM_MINI
	OBJECTS += $(PB_BD)/readline_simple.o
else
	OBJECTS += $(PB_BD)/readline.o
endif

#CFLAGS+=-flto
CFLAGS+=-ffat-lto-objects

all: $(PB_BD)/pogobios.bin

pogolib.a:
	cd $(POGOLIB_DIR) && $(MAKE)

INCLUDES+=-Iinclude -I$(POGOBIOS_DIR) -I$(BIOS_DIRECTORY) -I$(POGOLIB_DIR)

# pull in dependency info for *existing* .o files
-include $(OBJECTS:.o=.d)

$(PB_BD)/%.bin: $(PB_BD)/%.elf
	$(OBJCOPY) -O binary $< $@
	chmod -x $@

$(PB_BD)/pogobios.elf: $(OBJECTS) pogolib.a
	$(CC) $(LDFLAGS) \
		-T $(POGOBIOS_DIR)/linker.ld \
		-N -o $@ \
		$(OBJECTS) \
		-Wl,--whole-archive \
		-Wl,--gc-sections \
	   	-L$(BUILD_DIR)/software/libcompiler_rt -lcompiler_rt \
		-L$(BUILD_DIR)/software/libc -lc \
		-L$(BUILD_DIR)/software/pogolib -lpogobot

	chmod -x $@

main.o: $(POGOBIOS_DIR)/main.c
	@echo "Hint: make V=1 ... for verbose mode"
	@mkdir -p $(@D)
	$(compile)

$(PB_BD)/%.o: $(POGOBIOS_DIR)/%.c
	@mkdir -p $(@D)
	$(compile)

$(PB_BD)/%.o: $(POGOBIOS_DIR)/cmds/%.c
	$(compile)

$(PB_BD)/%.o: $(BIOS_DIRECTORY)/%.c
	$(compile)

$(PB_BD)/%.o: $(BIOS_DIRECTORY)/cmds/%.c
	$(compile)

$(PB_BD)/memtest.o: $(LIBBASE_DIRECTORY)/memtest.c
	$(compile)

$(PB_BD)/crc16.o: $(LIBBASE_DIRECTORY)/crc16.c
	$(compile)

$(PB_BD)/crc32.o: $(LIBBASE_DIRECTORY)/crc32.c
	$(compile)

$(PB_BD)/system.o: $(LIBBASE_DIRECTORY)/system.c
	$(compile)

$(PB_BD)/spiflash.o: $(LIBBASE_DIRECTORY)/spiflash.c
	$(compile)

$(PB_BD)/%.o: $(POGOBIOS_DIR)/%.S
	$(assemble)

$(PB_BD)/boot-helper.o: $(CPU_DIRECTORY)/boot-helper.S
	$(assemble)

clean:
	$(RM) $(OBJECTS) $(OBJECTS:.o=.d) pogobios.elf pogobios.bin .*~ *~
	cd $(POGOLIB_DIR) && $(MAKE) clean

.PHONY: all main.o

.PHONY: clang-format
clang-format:
	find . -type f -a \( -iname "*.c" -o -iname "*.h" \) -print | while read N ; do grep clang-format-ok "$$N" -q || continue ; echo "$$N" ; clang-format -i "$$N" ; done
