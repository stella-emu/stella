MODULE := src/emucore/rsynth

MODULE_OBJS := \
        src/emucore/rsynth/charset.o \
        src/emucore/rsynth/darray.o \
        src/emucore/rsynth/deutsch.o \
        src/emucore/rsynth/elements.o \
        src/emucore/rsynth/english.o \
        src/emucore/rsynth/holmes.o \
        src/emucore/rsynth/opsynth.o \
        src/emucore/rsynth/phones.o \
        src/emucore/rsynth/phtoelm.o \
        src/emucore/rsynth/say.o \
        src/emucore/rsynth/text.o \
        src/emucore/rsynth/trie.o

MODULE_DIRS += \
        src/emucore/rsynth

# Include common rules
include $(srcdir)/common.rules
