MODULE := src/emucore/elf

MODULE_OBJS = \
	src/emucore/elf/ElfParser.o

MODULE_DIRS += \
	src/emucore/elf

# Include common rules
include $(srcdir)/common.rules
