MODULE := src/emucore/tia/core_6502ts

MODULE_OBJS := \
	src/emucore/tia/core_6502ts/TIA.o \
	src/emucore/tia/core_6502ts/DelayQueueMember.o \
	src/emucore/tia/core_6502ts/DelayQueue.o \
	src/emucore/tia/core_6502ts/FrameManager.o \
	src/emucore/tia/core_6502ts/Playfield.o \
	src/emucore/tia/core_6502ts/DrawCounterDecodes.o \
	src/emucore/tia/core_6502ts/Missile.o \
	src/emucore/tia/core_6502ts/Player.o \
	src/emucore/tia/core_6502ts/Ball.o \
	src/emucore/tia/core_6502ts/LatchedInput.o


MODULE_DIRS += \
	src/emucore/tia/core_6502ts

# Include common rules
include $(srcdir)/common.rules
