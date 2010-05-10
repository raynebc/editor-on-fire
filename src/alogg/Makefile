# Makefile for AllegroOGG using Ogg Vorbis (Xiph.org Foundation)


# select (uncomment) one target and comment DJGPP if you are not aiming
# for that platform
#TARGET=DJGPP_STATIC
TARGET=MINGW32_STATIC
#TARGET=MINGW32_DLL
#TARGET=LINUX_STATIC


CC=gcc
LFLAGS=
LIBIMP=



# DJGPP_STATIC
ifeq ($(TARGET),DJGPP_STATIC)
TARGETFLAGS=-Wall -O2 -march=pentium -fomit-frame-pointer -finline-functions -ffast-math
OBJDIR=obj/djgpp/static
LIBDEST=lib/djgpp/libalogg.a
endif



# MINGW32_STATIC
ifeq ($(TARGET),MINGW32_STATIC)
LFLAGS=-mwindows
TARGETFLAGS=-Wall -O2 -march=pentium -fomit-frame-pointer -finline-functions -ffast-math
OBJDIR=obj/mingw32/static
LIBDEST=lib/mingw32/libalogg.a
endif


# MINGW32_DLL
ifeq ($(TARGET),MINGW32_DLL)
LFLAGS=-mwindows -shared
TARGETFLAGS=-Wall -O2 -march=pentium -fomit-frame-pointer -finline-functions -ffast-math
OBJDIR=obj/mingw32/dll
LIBIMP=lib/mingw32/libaloggdll.a
LIBDEST=lib/mingw32/alogg.dll
ALOGG_DLL=1
ALOGG_DLL_EXPORTS=1
endif



# LINUX_STATIC
ifeq ($(TARGET),LINUX_STATIC)
TARGETFLAGS=-Wall -O2 -march=pentium -fomit-frame-pointer -finline-functions -ffast-math
OBJDIR=obj/linux/static
LIBDEST=lib/linux/libalogg.a
endif



# setting object files, paths and compiler flags
vpath %.c decoder decoder/libvorbis/lib decoder/libogg/src

vpath %.c src
vpath %.o $(OBJDIR)

CFLAGS=$(TARGETFLAGS) -Idecoder -Iinclude -Idecoder/libvorbis/include -Idecoder/libogg/include
OBJECTS=analysis.o bitrate.o bitwise.o block.o codebook.o envelope.o floor0.o floor1.o framing.o info.o lpc.o lsp.o mapping0.o mdct.o psy.o registry.o res0.o sharedbook.o smallft.o synthesis.o vorbisfile.o window.o alogg.o
OBJECTS2=$(addprefix $(OBJDIR)/,$(OBJECTS))



# making of the library

# MINGW32_DLL
ifeq ($(TARGET),MINGW32_DLL)
$(LIBDEST): $(OBJECTS)
	$(CC) -o $@ $(OBJECTS2) $(LFLAGS) $(CFLAGS) -Wl,--out-implib=$(LIBIMP) -lalleg

# others
else
$(LIBDEST): $(OBJECTS)
	ar rs $(LIBDEST) $(OBJECTS2)
endif


# compiling of the library
%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $(OBJDIR)/$@


clean:
	rm -f $(OBJECTS2) $(LIBDEST) $(LIBIMP)

