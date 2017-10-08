MODULE := src/emucore/tia/frame-manager

MODULE_OBJS := \
	src/emucore/tia/frame-manager/FrameManager.o \
	src/emucore/tia/frame-manager/VblankManager.o \
	src/emucore/tia/frame-manager/AbstractFrameManager.o \
	src/emucore/tia/frame-manager/FrameLayoutDetector.o

MODULE_DIRS += \
	src/emucore/tia/frame-manager

# Include common rules
include $(srcdir)/common.rules
