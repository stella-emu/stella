MODULE := src/emucore/tia

MODULE_OBJS := \
	src/emucore/tia/TIA.o \
	src/emucore/tia/FrameManager.o \
	src/emucore/tia/Playfield.o \
	src/emucore/tia/DrawCounterDecodes.o \
	src/emucore/tia/Missile.o \
	src/emucore/tia/Player.o \
	src/emucore/tia/Ball.o \
	src/emucore/tia/Background.o \
	src/emucore/tia/LatchedInput.o \
	src/emucore/tia/PaddleReader.o \
	src/emucore/tia/VblankManager.o

MODULE_DIRS += \
	src/emucore/tia

# Include common rules
include $(srcdir)/common.rules
