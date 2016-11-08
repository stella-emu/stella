MODULE := src/emucore/tia/core_6502ts

MODULE_OBJS := \
	src/emucore/tia/core_6502ts/TIA.o

MODULE_DIRS += \
	src/emucore/tia/core_6502ts

# Include common rules 
include $(srcdir)/common.rules
