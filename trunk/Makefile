UNAME=$(shell uname)
ifneq (,$(findstring Linux,$(UNAME)))
SYSTEM=linux
else
ifneq (,$(findstring Darwin,$(UNAME)))
SYSTEM=macosx
else
ifneq (,$(findstring MINGW,$(UNAME)))
SYSTEM=mingw
else
$(warning Not sure which system you are on, assuming Linux)
$(warning   (uname says: '$(UNAME)'))
SYSTEM=linux
endif
endif
endif

top:
	$(MAKE) -C src -f makefile.$(SYSTEM)

clean:
	$(MAKE) -C src -f makefile.$(SYSTEM) clean
