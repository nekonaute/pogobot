TARGET ?=pogobotv3
BUILD_DIR=../build/$(TARGET)
POGOLIB_BUILDDIR=$(BUILD_DIR)/software/pogolib
POGOLIB_DIR=./

include $(BUILD_DIR)/software/include/generated/variables.mak
include $(SOC_DIRECTORY)/software/common.mak

OBJECTS = $(POGOLIB_BUILDDIR)/adc.o					\
		$(POGOLIB_BUILDDIR)/imu.o						\
		$(POGOLIB_BUILDDIR)/ir_uart.o					\
		$(POGOLIB_BUILDDIR)/pogobot_ir.o				\
		$(POGOLIB_BUILDDIR)/pogobot_timer_sleep_stopwatch.o	\
		$(POGOLIB_BUILDDIR)/rgb.o						\
		$(POGOLIB_BUILDDIR)/sleep.o					\
		$(POGOLIB_BUILDDIR)/spi.o						\
		$(POGOLIB_BUILDDIR)/ts4231.o					\
		$(POGOLIB_BUILDDIR)/pogolib.o					\
		$(POGOLIB_BUILDDIR)/pogolib_infrared.o			\
		$(POGOLIB_BUILDDIR)/slip.o						\
	    $(POGOLIB_BUILDDIR)/uart.o                     \
	    $(POGOLIB_BUILDDIR)/console.o                  \
		$(POGOLIB_BUILDDIR)/magnetometer.o                  \

OBJECTS += $(POGOLIB_BUILDDIR)/crt0.o

all: $(POGOLIB_BUILDDIR)/libpogobot.a

# Retain every API in the archive, with native code for non-LTO application links.
# Only each final executable decides which functions survive garbage collection.
LTO ?= 1
CFLAGS += -ffunction-sections -fdata-sections -ffat-lto-objects
ifeq ($(LTO),1)
CFLAGS += -flto
endif

# Rebuild objects when the compiler options above change.
$(OBJECTS): Makefile

INCLUDES+=-I$(POGOBIOS_DIR) -I$(BIOS_DIRECTORY) -I$(POGOLIB_DIR)

# pull in dependency info for *existing* .o files
-include $(OBJECTS:.o=.d)

$(POGOLIB_BUILDDIR)/libpogobot.a: $(OBJECTS)
	# The compiler's archiver records LTO symbols using its matching plugin.
	$(AR) -rc $@ $(OBJECTS)
$(info TARGET_PREFIX : $(TARGET_PREFIX))

$(POGOLIB_BUILDDIR)/%.o: $(POGOBIOS_DIR)/%.c
	@mkdir -p $(@D)
	$(compile)

$(POGOLIB_BUILDDIR)/%.o: $(POGOLIB_DIR)/%.c
	@mkdir -p $(@D)
	$(compile)

$(POGOLIB_BUILDDIR)/uart.o: $(LIBBASE_DIRECTORY)/uart.c
	$(compile)

$(POGOLIB_BUILDDIR)/console.o: $(LIBBASE_DIRECTORY)/console.c
	$(compile)

$(POGOLIB_BUILDDIR)/%.o: $(POGOLIB_DIR)/%.S
	$(assemble)

clean:
	$(RM) $(OBJECTS) $(OBJECTS:.o=.d) *.a .*~ *~

.PHONY: all libpogobot.a

.PHONY: clang-format
clang-format:
	find . -type f -a \( -iname "*.c" -o -iname "*.h" \) -print | while read N ; do grep clang-format-ok "$$N" -q || continue ; echo "$$N" ; clang-format -i "$$N" ; done
