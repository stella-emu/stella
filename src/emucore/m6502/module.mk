MODULE := src/emucore/m6502

MODULE_OBJS := \
	src/emucore/m6502/Device.o \
	src/emucore/m6502/M6502.o \
	src/emucore/m6502/NullDev.o \
	src/emucore/m6502/System.o

MODULE_DIRS += \
	src/emucore/m6502

# Include common rules 
include $(srcdir)/common.rules
