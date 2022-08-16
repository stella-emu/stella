MODULE := src/nanojpeg

MODULE_OBJS := \
	src/nanojpeg/nanojpeg.o

MODULE_DIRS += \
	src/nanojpeg

# Include common rules
include $(srcdir)/common.rules
