MODULE := src/emucore/speakjet

MODULE_OBJS := \
	src/emucore/speakjet/SpeakJet.o \
	src/emucore/speakjet/SpeakJetMSA.o \
	src/emucore/speakjet/SpeakJetSerial.o \
	src/emucore/speakjet/SpeakJetStream.o \
	src/emucore/speakjet/SpeakJetSynth.o \
	src/emucore/speakjet/SpeakJetTables.o

MODULE_TEST_OBJS =

MODULE_DIRS += \
	src/emucore/speakjet

# Include common rules
include $(srcdir)/common.rules
