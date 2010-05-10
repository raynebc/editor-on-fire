# CROSSCOMPILE: If defined, prepend XPREFIX to binaries.
# DJDIR: If defined, assume DJGPP compiler.
# MINGDIR: If defined, assume MINGW compiler.
# DEBUGMODE: If defined, compile with debugging info enabled.
# UNIXTOOLS: If defined, never use DOS/Windows tools.

.DEFAULT: all
.PHONY: all agup_lib make_dirs

all: agup_lib examples

CFLAGS := -W -Wall
ARFLAGS := rucs

ifdef DEBUGMODE
    CFLAGS += -g -DDEBUGMODE
else
    CFLAGS += -O2
endif

ifdef CROSSCOMPILE
    CC := $(XPREFIX)gcc
    AR := $(XPREFIX)ar
else
    CC := gcc
    AR := ar
endif

ifdef DJDIR
# Compile with DJGPP.
ifdef DEBUGMODE
    EXLIBS := -lalld
else
    EXLIBS := -lalleg
    EXAMP_LDFLAGS += -s
endif
    EXE := .exe
    INSTALL_PATH := $(DJDIR)
    INSTALL := copy
    RM := del
    RMDIR := del
    MKDIR := mkdir
else
ifdef MINGDIR
# Compile with MINGW.
  ifdef DEBUGMODE
    EXLIBS := -lalld
  else
    EXLIBS := -lalleg
    EXAMP_LDFLAGS += -s
  endif
    EXE := .exe
    INSTALL_PATH := $(MINGDIR)
    EXAMP_LDFLAGS += -mwindows
    RM := del
    RMDIR := del
    INSTALL := copy
    MKDIR := mkdir
    CFLAGS += -DMINGW

else
# Assume Unix.
   ALLEGRO = allegro-config
   ifdef DEBUGMODE
	CFLAGS += `$(ALLEGRO) --cflags`
	EXLIBS := `$(ALLEGRO) --libs debug`
   else
	CFLAGS += `$(ALLEGRO) --cflags`
	EXLIBS := `$(ALLEGRO) --libs`
	EXAMP_LDFLAGS += -s
   endif
	INSTALL_PATH := /usr/local
	RM := rm -f
	RMDIR := rm -r
    INSTALL := cp
    MKDIR := mkdir -p
    UNIXTOOLS=1
endif
endif

ifdef UNIXTOOLS
make_dirs:
	mkdir -p obj/release
	mkdir -p obj/debug	
	mkdir -p lib/release
	mkdir -p lib/debug

install:
	$(INSTALL) lib/release/libagup.a $(INSTALL_PATH)/lib
	$(INSTALL) lib/debug/libagupd.a $(INSTALL_PATH)/lib
	$(INSTALL) agup.h $(INSTALL_PATH)/include

uninstall:
	$(RM) $(INSTALL_PATH)$/lib/libagup*.a
	$(RM) $(INSTALL_PATH)$/include/agup.h
else
make_dirs:
	mkdir obj
	mkdir obj\release
	mkdir obj\debug
	mkdir lib
	mkdir lib\release
	mkdir lib\debug

install:
	$(INSTALL) lib\release\libagup.a $(INSTALL_PATH)\lib
	$(INSTALL) lib\debug\libagupd.a $(INSTALL_PATH)\lib
	$(INSTALL) agup.h $(INSTALL_PATH)\include

uninstall:
	$(RM) $(INSTALL_PATH)\lib\libagup.a
	$(RM) $(INSTALL_PATH)\lib\libagupd.a
	$(RM) $(INSTALL_PATH)\include\agup.h
endif

ifdef DEBUGMODE
AGUP_LIB := libagupd.a
SUB_DIR := debug
else
AGUP_LIB := libagup.a
SUB_DIR := release
endif

AGUP_OBJS := $(addprefix obj/$(SUB_DIR)/, \
             agup.o agtk.o aphoton.o awin95.o aase.o \
             abeos.o ans.o aalg.o abitmap.o \
             )

agup_lib: make_dirs lib/$(SUB_DIR)/$(AGUP_LIB)

lib/$(SUB_DIR)/$(AGUP_LIB): $(AGUP_OBJS)
	$(AR) $(ARFLAGS) $@ $^

obj/$(SUB_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

EXAMP := examp$(EXE)

examples: $(EXAMP)

$(EXAMP): obj/$(SUB_DIR)/examp.o lib/$(SUB_DIR)/$(AGUP_LIB)
	$(CC) $(CFLAGS) $(EXAMP_LDFLAGS) -o $(EXAMP) $^ $(EXLIBS) 

clean:
	-$(RM) $(EXAMP)
	-$(RMDIR) lib	
	-$(RMDIR) obj

