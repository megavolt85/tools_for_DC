TARGET = AFS_Tools
OBJS = src/main.c
OBJS_GUI = src/ui.cxx src/win.cpp

#for linux
CC_LINUX = g++
LIB_LINUX_GUI = -lfltk -Isrc/ -lX11 -lXft -lXext -lXfixes -lfontconfig -ldl -lm -lstdc++

#for win32
CC_WIN32= i586-mingw32msvc-g++
LIB_WIN32= -I/usr/i586-mingw32msvc/include -L/usr/i586-mingw32msvc/lib -lmingw32 -luser32 -lgdi32 -lwinmm -ldxguid -lwldap32 -lws2_32 -lstdc++
#-static -static-libstdc++ -static-libgcc
LIB_WIN32_GUI = -lfltk -lole32 -luuid -lcomctl32 -Isrc/
CFLAGS_WIN32 = -mwindows


all: clean linux win32 linux_gui win32_gui

linux:
# Compiling for linux
	$(CC_LINUX) -m64 $(OBJS) -o $(TARGET) $(LIBS) -DNO_GUI
#	strip $(TARGET)

win32: resources.o
# Compiling for win32
	$(CC_WIN32) $(OBJS) src/resources.o -o $(TARGET).exe $(LIB_WIN32) -DNO_GUI
#	i586-mingw32msvc-strip $(TARGET).exe

linux_gui:
# Compiling for linux
	$(CC_LINUX) -m64 $(OBJS) $(OBJS_GUI) -o $(TARGET)_gui $(LIBS) $(LIB_LINUX_GUI) -Wno-deprecated-declarations
	strip $(TARGET)_gui

win32_gui: resources.o
# Compiling for win32
	$(CC_WIN32) $(OBJS) $(OBJS_GUI) src/resources.o -o $(TARGET)_gui.exe $(LIB_WIN32_GUI) $(LIB_WIN32) -Wno-deprecated-declarations
	i586-mingw32msvc-strip $(TARGET)_gui.exe

clean:
	rm -f $(TARGET) $(TARGET).exe $(TARGET)_gui $(TARGET)_gui.exe src/resources.o

resources.o:
	i586-mingw32msvc-windres src/winres.rc -O coff -o src/resources.o
