MODULE := src/common

MODULE_OBJS := \
	src/common/mainSDL.o \
	src/common/Base.o \
	src/common/SoundSDL.o \
	src/common/FrameBufferSoft.o \
	src/common/FrameBufferGL.o \
	src/common/FBSurfaceGL.o \
	src/common/FBSurfaceTIA.o \
	src/common/FSNodeZIP.o \
	src/common/PNGLibrary.o \
	src/common/MouseControl.o \
	src/common/RectList.o \
	src/common/ZipHandler.o

MODULE_DIRS += \
	src/common

# Include common rules 
include $(srcdir)/common.rules
