MODULE := src/common

MODULE_OBJS := \
	src/common/main.o \
	src/common/Base.o \
	src/common/EventHandlerSDL2.o \
	src/common/FrameBufferSDL2.o \
	src/common/FBSurfaceSDL2.o \
	src/common/SoundSDL2.o \
	src/common/FSNodeZIP.o \
	src/common/PNGLibrary.o \
	src/common/MouseControl.o \
	src/common/ZipHandler.o

MODULE_DIRS += \
	src/common

# Include common rules 
include $(srcdir)/common.rules
