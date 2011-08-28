MODULE := src/common

MODULE_OBJS := \
	src/common/mainSDL.o \
	src/common/SoundNull.o \
	src/common/SoundSDL.o \
	src/common/FrameBufferSoft.o \
	src/common/FrameBufferGL.o \
	src/common/FBSurfaceGL.o \
	src/common/FBSurfaceTIA.o \
	src/common/PNGLibrary.o \
	src/common/RectList.o \
	src/common/Snapshot.o

MODULE_DIRS += \
	src/common

# Include common rules 
include $(srcdir)/common.rules
