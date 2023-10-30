# Mac Build Instructions

Instructions for building emulator on MacOS.

Emulator uses following libraries:
* [SDL2](https://www.libsdl.org/) library to implement integration with display, sound and input devices.
* [gettext](https://www.gnu.org/software/gettext/) library to implement internationalization.

To install those libraries using [Homebrew](https://brew.sh/) issue this command:
```shell
brew instal sdl2 gettext
```

Find where SDL library was installed, it should look something like that `/usr/local/Cellar/sdl2/2.28.4` although version number
will likely be different. This location will be needed for adjustments in the `Makefile`.

Open `Makefile` for editing and make following changes:
* Change `CC` variable definition to add location of the include folder of the SDL2
  library, assuming library is located at same location as above add this ` -I/usr/local/Cellar/sdl2/2.28.4/include`.
* Replace `/usr/lib/x86_64-linux-gnu/libSDL.so` with `/usr/local/lib/libSDL2.dylib`
* Uncomment line `GETTEXT_LIB = /usr/local/lib/libintl.dylib`

Now if you run make it should compile successfully.

You should be able to start emulator by issuing command:
```shell
BK_PATH=Rom ./bk -1 -c
```