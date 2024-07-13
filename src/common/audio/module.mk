MODULE := src/common/audio

MODULE_OBJS := \
	src/common/audio/SimpleResampler.o \
	src/common/audio/ConvolutionBuffer.o \
	src/common/audio/LanczosResampler.o \
	src/common/audio/HighPass.o

MODULE_TEST_OBJS =

MODULE_DIRS += \
	src/emucore/tia \
	src/emucore/elf

# Include common rules
include $(srcdir)/common.rules
