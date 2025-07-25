BK emulator
==========

DISCLAIMER
----------

I (emestee@catnarok.net) am not the author of this software. The original code for the PDP11 emulator was
written by Eric A. Edwards in 1994. It was then modified by Leonid A. Broukhis to support the Soviet BK-0010, 
BK-0010.01 and BK-0011(M). 

The original license statement in Eric A. Edwards' source code is as follows:

> Permission to use, copy, modify, and distribute this software and its documentation for any purpose and without fee is
> hereby granted, provided that the above copyright notice appear in all copies.  Eric A. Edwards makes no representations 
> about the suitability of this software for any purpose.  It is provided "as is" without expressed or implied warranty.

Leonid A. Broukhis preserved this license in his source code, and therefore this is free software.

My hat is off before these two gentlemen.

What is this?
-------------

This software is a Linux/SDL emulator for Soviet (russian) Electronica BK series:

* БК-0010
* БК-0010.01
* БК-0011(М)

You may read more about the series at http://en.wikipedia.org/wiki/Electronika_BK

The author's (Leonid A. Broukhis) page can be found at http://www.mailcom.com/bk0010/index_en.shtml . Unfortunately the
website appears abandoned and the link to source code for this project is broken.

Additionally, it supports emulation of Terak 8510/a, which is a 1976 american PDP-11/03 platform and of which
the Electronica BK series are indeed clones.

The Electronica BK computers were the first mass produced, affordable personal computers in Russia in the eighties of 20th century,
and have had tremendous effect on the development of Russian-speaking software community. Every russian computer, hardware or ham radio
geek born in seventies or eighties probably had one of those machines during their formational years. They are the C64s, ZX Spectrums
and Atari 2600s of the now defunct USSR. 

I have discovered this software by accident, and I am publishing it to github in order to preserve it.

**All of the BK machines accept console commands in English, but respond mostly in Russian. Most of the software written for
these machines would also interact in Russian. Unless you are a native Russian speaker, or very curious about this technology,
this software is probably of very little use to you**

The version of the emulator published in this repository is derived from "BK-Terak-Emu 2005.08.26".tar.gz file. I do not know
if a more up to date version was ever made, or has been preserved. The original file is included unmodified in this repository.
The only alteration I made to the repository is a change to the Makefile to point the linker to libSDL as it appears on 
modern x64 machines.

Virtually all of BK schematics, documentation, hardware hacks and software has been preserved and is available on the internet.
Unfortunately, most if not all of these things are in Russian.

Compilation and use
-------------------

For instructions on how to compile on Mac please read [MAC-BUILD.md](./MAC-BUILD.md)

* Make sure libsdl2 and its -dev packages are installed
* Run `make`
* Run `./bk -?` for overview of emulation modes

For instance, to run BK-0010.01 emulation, execute

`./bk -1 -c`

Some games can be downloaded [here](http://bk0010.narod.ru/). The original authors generally do not object to redistribution of their work, as most of it was written while copyright law in Russia was non-existent.

Games are published in two binary formats that are slightly different: the `bkread` format (as is the case with the above website) and the raw tape format. The `bkread` format
embeds a BK-internal file name that this emulator does not understand. Use the script `convert_bkread.py` to check and convert binary files.

To load a game binary from within the emulator, do:

* Type `mon` and press enter in the BASIC shell, this will throw you into the monitor mode
* Type `m` and press enter 
* Input the file name, case-sensitive, in the current working directory. If "ОШИБКА" is displayed, check the file name 
or try converting the file
* After the file is loaded and you receive a prompt, type `s` and press enter

The following is supposed to work, but sometimes doesn't:

When the emulation is running, you can issue a Ctrl-C in the emulator's console. This will stop the emulation and drop
you into the emulator's debugger. To load a game from a binary file (assuming you have one, none are distributed here), do:

`l <filename>`
`g 1000`


You can also press F12 when the emulator is running and input a binary file name in the emulator console window.
