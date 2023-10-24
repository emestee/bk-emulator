#
# This file is part of 'pdp', a PDP-11 simulator.
#
# For information contact:
#
#   Computer Science House
#   Attn: Eric Edwards
#   Box 861
#   25 Andrews Memorial Drive
#   Rochester, NY 14623
#
# Email:  mag@potter.csh.rit.edu
# FTP:    ftp.csh.rit.edu:/pub/csh/mag/pdp.tar.Z
# 
# Copyright 1994, Eric A. Edwards
#
# Permission to use, copy, modify, and distribute this
# software and its documentation for any purpose and without
# fee is hereby granted, provided that the above copyright
# notice appear in all copies.  Eric A. Edwards makes no
# representations about the suitability of this software
# for any purpose.  It is provided "as is" without expressed
# or implied warranty.
#
# Makefile
#
#

#
# Change as needed
#

CC = gcc -std=gnu89 -Wno-error=implicit-function-declaration -Wno-error=return-type # -I/usr/local/Cellar/sdl2/2.28.4/include
LD = gcc
CFLAGS = -g -DSHIFTS_ALLOWED -DEIS_ALLOWED
# CFLAGS = -O4 -fomit-frame-pointer # -DSHIFTS_ALLOWED

#
# Targets
#

TARGET = bk
UTILS = maketape readtape

#
# Source and Object Files
#

SRCS =	access.c boot.c branch.c conf.c covox.c double.c ea.c itab.c \
	main.c service.c ui.c scr.c timer.c tape.c disk.c mouse.c printer.c \
	single.c weird.c tty.c io.c timing.c sound.c disas.c serial.c bkplip.c \
	terakdisk.c synth.c emu2149.c
OBJS =	access.o boot.o branch.o conf.o covox.o double.o ea.o itab.o icon.o \
	main.o service.o ui.o scr.o timer.o tape.o disk.o mouse.o printer.o \
	single.o weird.o tty.o io.o timing.o sound.o disas.o serial.o bkplip.o \
	terakdisk.o synth.o emu2149.o
INCS =	defines.h scr.h conf.h emu2149.h emutypes.h
USRCS = readtape.c maketape.c pngtorgba.c
TEXTS =	README.html configure.in icon.c
# GETTEXT_LIB = /usr/local/lib/libintl.dylib

#
# Build Rules
#

everything:	$(TARGET) $(UTILS)
	touch everything

.c.o:
	$(CC) -c $(CFLAGS) $<

icon.c: pngtorgba bk.png
	touch icon.c
	if [ ! -s icon.c ] ; then ./pngtorgba bk.png > icon.c ; fi

$(TARGET):	$(OBJS)
	$(LD) $(CFLAGS) -o $(TARGET) $(OBJS) /usr/lib/x86_64-linux-gnu/libSDL.so $(GETTEXT_LIB) -lpthread

readtape: readtape.c
	$(CC) $(CFLAGS) -o readtape $(GETTEXT_LIB) readtape.c

maketape: maketape.c
	$(CC) $(CFLAGS) -o maketape $(GETTEXT_LIB) maketape.c

#
# Cool Utilities
#

clean:
	rm -f $(TARGET) $(OBJS) $(UTILS) everything

count:
	wc -l $(SRCS) $(USRCS)

dist:
	tar czvf bk-terak-emu.`date +%Y.%m.%d`.tar.gz \
	--exclude CVS $(SRCS) $(INCS) $(USRCS) po Rom $(TEXTS) Makefile bk.png

depend:
	makedepend -Dlinux=1 -Y $(SRCS) $(USRCS) $(INCS)

# DO NOT DELETE

access.o: defines.h
boot.o: defines.h
branch.o: defines.h
conf.o: conf.h defines.h scr.h
covox.o: defines.h
double.o: defines.h
ea.o: defines.h
itab.o: defines.h
main.o: defines.h scr.h
service.o: defines.h
ui.o: defines.h
scr.o: defines.h scr.h
timer.o: defines.h
tape.o: defines.h
disk.o: defines.h
mouse.o: defines.h
printer.o: defines.h
single.o: defines.h
weird.o: defines.h
tty.o: defines.h
io.o: defines.h
timing.o: defines.h
sound.o: defines.h
disas.o: defines.h
serial.o: defines.h
bkplip.o: defines.h
terakdisk.o: defines.h
synth.o: defines.h emu2149.h emutypes.h
emu2149.o: emu2149.h emutypes.h
conf.o: defines.h
emu2149.o: emutypes.h
