MODULE := src/common/television

MODULE_OBJS := \
	src/common/television/Television.o \
	src/common/television/PaletteHandler.o \
	src/common/television/PhosphorHandler.o \
	src/common/television/TVSignal.o \
	src/common/television/NTSCSignal.o \
	src/common/television/PALSignal.o \
	src/common/television/AtariNTSC.o

MODULE_TEST_OBJS =

MODULE_DIRS += \
	src/common/television

# Include common rules
include $(srcdir)/common.rules
