ifeq ($(OS),Windows_NT)
 UNAME=$(shell uname)
 ifneq (,$(findstring MINGW,$(UNAME)))
  SYSTEM=msys
 else
  SYSTEM=mingw
 endif
else
 UNAME=$(shell uname)
 ifneq (,$(findstring Linux,$(UNAME)))
  SYSTEM=linux
 else
  ifneq (,$(findstring Darwin,$(UNAME)))
   SYSTEM=macosx
  else
   ifneq (,$(findstring MINGW,$(UNAME)))
    SYSTEM=mingw_cross
   else
    $(warning Not sure which system you are on, assuming Linux)
    $(warning   (uname says: '$(UNAME)'))
    SYSTEM=linux
   endif
  endif
 endif
endif

top:
	$(MAKE) -C src -f makefile.$(SYSTEM)

clean:
	$(MAKE) -C src -f makefile.$(SYSTEM) clean

legacy:
	$(MAKE) -C src -f makefile.legacy
