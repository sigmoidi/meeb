# MEEB
> _The **M**inimal **E**x**e**cutable **B**uild Tool_

A small program for building minimal Windows executables. Tech-wise, this is nothing fancy - MEEB simply relays calls to other tools. The point of MEEB is to reduce the average command line length for compiling and remove the need for that annoying `build.bat` / Makefile / whatever.

It is 2024 and yet, building from the command line on Windows is still a pain. Sure, one can simply run `vcvars64` and `cl.exe program.c blah blah`, but this feels bloated and messy. What's more, if you want to use GCC on WSL for compiling, you have to either move your whole build chain to Linux/WSL or hop between CMD and Bash shells. Not fun.

Furthermore, executables are often extremely bloated by default, with a basic "Hello, world!" program taking up **several hundred kilobytes**. This is often due to libraries that are baked into the executable, and at least in my personal development experience, most of it is unnecessary. Sure, the C standard library is nice, but if I need to code something for Windows, I might as well use the native functionality in the DLLs that are installed by default. And the standard library isn't available everywhere.

The primary use case for MEEB is small, standalone programs. The idea came up while I was working on a demoscene production, but the tool is suitable for other kinds of projects as well.

## Dependencies (tools)

The program itself has no dependencies, but for compression, MEEB knows how to use three external programs:

* [Crinkler](https://github.com/runestubbe/Crinkler)
* [kkrunchy](https://www.farbrausch.de/~fg/kkrunchy/)
* [UPX](https://upx.github.io/)

Since I don't own any of these, you have to install them yourself. Once you have acquired the executables, place them in `tools/[normalized_name].exe` - that is, for e.g. Crinkler, the file would be `tools/crinkler.exe`.

For MSVC and GCC, you need to install either (or both of):

* **Visual Studio Tools**, or equivalent, which contains `cl.exe` and `link.exe`
* **Linux Subsystem for Windows** and the `gcc-mingw-w64-x86-64-win32` package, which contains `x86_64-w64-mingw32-gcc-win32`

Note that for MSVC, you have to set the build environment up with your method of choice. Effectively this means adding the compiler and linker to path and telling them where the Windows toolkit/libraries are. I find it easiest to find and run `vcvars32.bat` or `vcvars64.bat`, depending on which bitness you want.

## Installation

None required :)

Simply download the MEEB executable, as well as any desired tools (see above).

## Usage

Usage instructions can be viewed with `meeb` or `meeb -h`:

```
C:\> meeb
 ____    ____  ________  ________  _______
|_   \  /   _||_   __  ||_   __  ||_   _  \
  |   \/   |    | |_ \_|  | |_ \_|  | |_) /
  | |\  /| |    |  _| _   |  _| _   |  __ \
 _| |_\/_| |_  _| |__/ | _| |__/ | _| |__) |
|_____||_____||________||________||_______/

==== The Minimal Executable Build Tool ====
  \_ Written By Henri A.   Version 1.0 _/

Usage: meeb source [options]
Options:
   -o   Output file
   -o!  Output file, force overwrite
        Defaults to 'out.exe', no force
   -c   Compiler, either 'msvc' or 'gcc'
        Defaults to 'msvc'
   -l   Linker, either 'msvc' or 'crinkler'
        Defaults to 'msvc'
   -x   Further compression, either 'upx' or 'kkrunchy'
        Defaults to none
   -f   Optimization goal, either 'fast' or 'small'
        Defaults to 'fast'
   -b   Executable bitness, either 32 or 64
        Defaults to 64
   -d   Don't strip or remove debugging information
        Defaults to stripped (no flag)

Note: You must install any desired tools beforehand.
      Please see the README for more information.
```

## Building

Sadly, MEEB cannot build itself yet. But there is a convenient build script. Simply set up an MSVC build environment and call the script with
```bash
> vcvars64
> build
```
