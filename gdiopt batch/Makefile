#for linux
CC_LINUX = gcc

#for win32
CC_WIN32= i586-mingw32msvc-g++
LIB_WIN32= -I/usr/i586-mingw32msvc/include -L/usr/i586-mingw32msvc/lib -lmingw32 -luser32 -lgdi32 -lole32 -luuid -lcomctl32

OBJS = gdiopt.c
TARGET = gdiopt

all: clean linux linux64 win32

linux:
# Compiling for linux
	$(CC_LINUX) -m32 $(OBJS) -o $(TARGET)_x32
	strip $(TARGET)_x32

linux64:
# Compiling for linux
	$(CC_LINUX) $(OBJS) -o $(TARGET)
	strip $(TARGET)

win32:
# Compiling for win32
	$(CC_WIN32) $(OBJS) -o $(TARGET).exe $(LIB_WIN32)
	i586-mingw32msvc-strip $(TARGET).exe

clean:
	rm -f $(TARGET)_x32 $(TARGET) $(TARGET).exe
