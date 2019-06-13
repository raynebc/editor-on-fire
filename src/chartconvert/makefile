CC = gcc
CFLAGS = -Wall -O2 -funroll-loops -mwindows -static-libgcc -static-libstdc++ -flto -Ialogg/include -static
CXXFLAGS = $(CFLAGS)
LIBS = -lalleg44.dll
OFILES = chartconvert.o

EXENAME = chartconvert.exe

all : $(EXENAME)

$(EXENAME) : $(OFILES)
	gcc $(OFILES) $(LIBS) -o $(EXENAME)

clean:
	@del *.o
	@del $(EXENAME)
	@echo Cleanup complete!
