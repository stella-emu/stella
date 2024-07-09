MODULE := src/emucore/elf

MODULE_OBJS = \
	src/emucore/elf/ElfParser.o \
	src/emucore/elf/ElfLinker.o \
	src/emucore/elf/ElfUtil.o

MODULE_DIRS += \
	src/emucore/elf

# Include common rules
include $(srcdir)/common.rules
