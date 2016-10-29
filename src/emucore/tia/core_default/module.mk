MODULE := src/emucore/tia/core_default

MODULE_OBJS := \
	src/emucore/tia/core_default/TIA.o \
	src/emucore/tia/core_default/TIATables.o

MODULE_DIRS += \
	src/emucore/tia/core_default

# Include common rules 
include $(srcdir)/common.rules
