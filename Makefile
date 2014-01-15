

StellarisWare=../sw
CompilerPath=${HOME}/arm/gcc-arm-none-eabi
CompilerBin=${CompilerPath}/bin

CC=${CompilerBin}/arm-none-eabi-gcc
CFLAGS=-mthumb -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=soft -Os -ffunction-sections -fdata-sections -MD -std=c11 -Wall -fno-builtin -pedantic -DPART_LM4F120H5QR -I${StellarisWare} -Dgcc -g

LD=${CompilerBin}/arm-none-eabi-ld 
LDFLAGS=-g

OBJCOPY=${CompilerBin}/arm-none-eabi-objcopy
OBJDUMP=${CompilerBin}/arm-none-eabi-objdump
OBJSIZE=${CompilerBin}/arm-none-eabi-size


# arm-none-eabi-ld -T blinky.ld --entry ResetISR --gc-sections -o gcc/blinky.axf gcc/blinky.o gcc/startup_gcc.o /home/eric/arm/gcc-arm-none-eabi-4_7-2013q3/bin/../lib/gcc/arm-none-eabi/4.7.4/../../../../arm-none-eabi/lib/armv7e-m/softfp/libm.a /home/eric/arm/gcc-arm-none-eabi-4_7-2013q3/bin/../lib/gcc/arm-none-eabi/4.7.4/../../../../arm-none-eabi/lib/armv7e-m/softfp/libc.a /home/eric/arm/gcc-arm-none-eabi-4_7-2013q3/bin/../lib/gcc/arm-none-eabi/4.7.4/armv7e-m/softfp/libgcc.a

OBJS=startup_gcc.o main.o eth.o mem.o uartstdio.o
LIBS=${StellarisWare}/driverlib/gcc-cm4f/libdriver-cm4f.a

ma.bin: ma.axf
	${OBJCOPY} -O binary ${@:.bin=.axf} ${@}

ma.axf: $(OBJS) Makefile ma.ld
	$(LD) -T ma.ld --entry ResetISR --gc-sections $(LDFLAGS) -o $@ $(OBJS) $(LIBS)
	$(OBJDUMP) -S -D $@ > ma.asm
	$(OBJSIZE) $@

.c.o:
	$(CC) $(CFLAGS) -c -o $*.o $*.c

uartstdio.o: ${StellarisWare}/utils/uartstdio.c
	$(CC) $(CFLAGS) -c -o $@ $<

main.o: acn.h mac.h
eth.o: acn.h mac.h acnraw.h

genframe.o: genframe.c acn.h
	g++ -c -o $@ $<

genframe: genframe.o
	g++ -o $@ $<

acnraw.h: genframe
	./genframe

flash: ma.bin
	SU lm4flash ma.bin

gdb: ma.bin
	${CompilerBin}/arm-none-eabi-gdb ma.axf

clean:
	rm -f ma.bin
	rm -f ma.axf
	rm -f *.o
	rm -f *.d
