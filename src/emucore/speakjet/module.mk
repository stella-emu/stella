MODULE := src/emucore/speakjet

MODULE_OBJS := \
	src/emucore/speakjet/SpeakJet.o \
	src/emucore/speakjet/SpeakJetDSP.o \
	src/emucore/speakjet/SpeakJetSamples.o \
	src/emucore/speakjet/SpeakJetSerial.o \
	src/emucore/speakjet/SpeakJetSoftware.o \
	src/emucore/speakjet/SpeakJetTables.o \
	src/emucore/speakjet/SpeakJetVoice.o

MODULE_TEST_OBJS =

MODULE_DIRS += \
	src/emucore/speakjet

# Include common rules
include $(srcdir)/common.rules
