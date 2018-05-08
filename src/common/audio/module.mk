MODULE := src/common/audio

MODULE_OBJS := \
	src/common/audio/SimpleResampler.o

MODULE_DIRS += \
	src/emucore/tia

# Include common rules
include $(srcdir)/common.rules
