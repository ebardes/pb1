
.SUFFIXES: .exe .obj .c .y

CC=i686-w64-mingw32-gcc

LISTENOBJS=listen.obj consolegram.obj Lib/WSock32.Lib

listen.exe: $(LISTENOBJS)
	$(CC) -o $@ $(LISTENOBJS)

listen.obj: listen.c ../acn.h ../acnraw.h

.y.c:
	bison --output=$*.c $*.y

.c.obj:
	$(CC) -I.. -O3 -o $*.obj -c $*.c
