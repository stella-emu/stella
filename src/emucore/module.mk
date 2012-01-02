MODULE := src/emucore

MODULE_OBJS := \
	src/emucore/AtariVox.o \
	src/emucore/Booster.o \
	src/emucore/Cart2K.o \
	src/emucore/Cart3F.o \
	src/emucore/Cart3E.o \
	src/emucore/Cart4A50.o \
	src/emucore/Cart4K.o \
	src/emucore/CartAR.o \
	src/emucore/CartCV.o \
	src/emucore/Cart.o \
	src/emucore/CartDPC.o \
	src/emucore/CartDPCPlus.o \
	src/emucore/CartE0.o \
	src/emucore/CartE7.o \
	src/emucore/CartEF.o \
	src/emucore/CartEFSC.o \
	src/emucore/CartF0.o \
	src/emucore/CartF4.o \
	src/emucore/CartF4SC.o \
	src/emucore/CartF6.o \
	src/emucore/CartF6SC.o \
	src/emucore/CartF8.o \
	src/emucore/CartF8SC.o \
	src/emucore/CartFA.o \
	src/emucore/CartFE.o \
	src/emucore/CartMC.o \
	src/emucore/CartSB.o \
	src/emucore/CartUA.o \
	src/emucore/Cart0840.o \
	src/emucore/CartX07.o \
	src/emucore/CompuMate.o \
	src/emucore/Console.o \
	src/emucore/Control.o \
	src/emucore/Driving.o \
	src/emucore/EventHandler.o \
	src/emucore/FrameBuffer.o \
	src/emucore/FSNode.o \
	src/emucore/Genesis.o \
	src/emucore/Joystick.o \
	src/emucore/Keyboard.o \
	src/emucore/KidVid.o \
	src/emucore/MindLink.o \
	src/emucore/M6502.o \
	src/emucore/M6532.o \
	src/emucore/MT24LC256.o \
	src/emucore/NullDev.o \
	src/emucore/MD5.o \
	src/emucore/OSystem.o \
	src/emucore/Paddles.o \
	src/emucore/Props.o \
	src/emucore/PropsSet.o \
	src/emucore/Random.o \
	src/emucore/SaveKey.o \
	src/emucore/Serializer.o \
	src/emucore/Settings.o \
	src/emucore/Switches.o \
	src/emucore/StateManager.o \
	src/emucore/System.o \
	src/emucore/TIA.o \
	src/emucore/TIASnd.o \
	src/emucore/TIATables.o \
	src/emucore/TrackBall.o \
	src/emucore/unzip.o \
	src/emucore/MediaFactory.o \
	src/emucore/Thumbulator.o

MODULE_DIRS += \
	src/emucore

# Include common rules 
include $(srcdir)/common.rules
