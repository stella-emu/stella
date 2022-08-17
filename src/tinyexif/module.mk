MODULE := src/tinyexif

MODULE_OBJS := \
	src/tinyexif/tinyexif.o

MODULE_DIRS += \
	src/tinyexif

# Include common rules
include $(srcdir)/common.rules
