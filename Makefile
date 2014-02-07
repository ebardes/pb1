

StellarisWare=../tw
CompilerPath=${HOME}/arm/gcc-arm-none-eabi
CompilerBin=${CompilerPath}/bin

CC=${CompilerBin}/arm-none-eabi-gcc
CFLAGS=-mthumb -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=soft -Os -ffunction-sections -fdata-sections -MD -std=c11 -Wall -fno-builtin -pedantic -DPART_TM4C123GH6PM -I${StellarisWare} -Dgcc -g

LD=${CompilerBin}/arm-none-eabi-ld 
LDFLAGS=-g

OBJCOPY=${CompilerBin}/arm-none-eabi-objcopy
OBJDUMP=${CompilerBin}/arm-none-eabi-objdump
OBJSIZE=${CompilerBin}/arm-none-eabi-size


all: ma.bin map.xml

OBJS=startup_gcc.o main.o eth.o mem.o usb_host_mouse.o uartstdio.o
LIBS= \
	${StellarisWare}/usblib/gcc/libusb.a \
	${StellarisWare}/driverlib/gcc/libdriver.a \

ma.bin: ma.axf
	${OBJCOPY} -O binary ${@:.bin=.axf} ${@}

ma.axf: $(OBJS) ma.ld
	$(LD) -T ma.ld --entry ResetISR --gc-sections $(LDFLAGS) -o $@ $(OBJS) $(LIBS)
	$(OBJDUMP) -S -D $@ > ma.asm
	$(OBJSIZE) $@

.c.o:
	$(CC) $(CFLAGS) -c -o $*.o $*.c

uartstdio.o: ${StellarisWare}/utils/uartstdio.c
	$(CC) $(CFLAGS) -c -o $@ $<

main.o: acn.h mac.h
eth.o: acn.h mac.h acnraw.h
mem.o: acn.h mac.h
usbmouse.o: acn.h mac.h

genframe.o: genframe.c acn.h
	g++ -c -o $@ $<

genframe: genframe.o
	g++ -o $@ $<

acnraw.h: genframe
	./genframe

flash: ma.bin
	SU lm4flash ma.bin

map.xml: map.rb map.xslt keypadmap.txt
	ruby map.rb keypadmap.txt | xsltproc -o map.xml map.xslt - 
	cat map.xml

gdb: ma.bin
	${CompilerBin}/arm-none-eabi-gdb ma.axf

clean:
	rm -f ma.bin
	rm -f ma.axf
	rm -f *.o
	rm -f *.d
