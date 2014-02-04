
.SUFFIXES: .exe .obj .c .y

CC=i686-w64-mingw32-gcc

LISTENOBJS=main.obj listen.obj consolegram.obj ma2gram.obj Lib/WSock32.Lib Lib/Gdi32.Lib

listen.exe: $(LISTENOBJS)
	$(CC) -Wl,--subsystem,windows -o $@ $(LISTENOBJS)

listen.obj: listen.c ../acn.h ../acnraw.h

.y.c:
	bison --xml=$*.xml --name-prefix=${*:gram=} --output=$*.c $*.y

.c.obj:
	$(CC) -I.. -O3 -o $*.obj -c $*.c
