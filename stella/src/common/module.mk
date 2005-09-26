MODULE := src/common

MODULE_OBJS := \
	src/common/Cheat.o \
	src/common/CheetahCheat.o \
	src/common/BankRomCheat.o \
	src/common/mainSDL.o \
	src/common/SoundNull.o \
	src/common/SoundSDL.o \
	src/common/FrameBufferSoft.o \
	src/common/FrameBufferGL.o \
	src/common/Snapshot.o

MODULE_DIRS += \
	src/common

# Include common rules 
include $(srcdir)/common.rules
