# robco-processor
> an emulated fantasy computer system inspired by the terminals found in the Fallout universe

A bit of an expansion on the ideas in https://github.com/skwirl42/robco-os including an ISA for a stack-based processor.

## Platforms:
- macOS 10.15+ with dev tools installed
  * Install SDL2 and SLD2_image to /Library/Frameworks in any way you choose
  * flex (latest) and bison (2.7) need to be installed via homebrew: https://brew.sh/
- Windows with Visual Studio installed
  * note: you'll need to add SDL2 and SLD2_image using nuget, as cmake's nuget support and VS C++ projects don't get along
  * this is best accomplished by installing sdl2_image first, as it will pick out the right SDL2 it needs
  * flex and bison should be installed from the GnuWin32 site http://gnuwin32.sourceforge.net/
- Could probably be easily ported to other platforms/versions, as code is straight C/C++ and SDL2

## Requirements:
- CMake 3.15+
- SDL2
- SDL2_image
- flex
- bison

## The Debugger
While running the emulator clicking in the window with the mouse will pause execution and bring up the debugging screen. It shows the status of the registers, the contents of the stack, the data pointed at by the index registers, and the instruction that last executed. To resume normal execution press F5.

## The Assembler
While the main executable assembles a program before executing it, you can use the "assembler" cmake target to make a standalone version of the assembler. The standalone assembler outputs a text file containing the hexadecimal code and data regions, and a list of symbols defined in the program. This file is not meant to be executed, but rather for debugging and testing purposes.

## TODO
- [ ] Write a programming guide
- [x] Better error handling for the assembler
- [ ] Update the debugger to support showing the next instruction to be executed, instead of the last
- [ ] Update the debugger to properly show the source arguments for register indexed instructions
- [ ] Allow for swapping between the debug and regular console while execution remains paused
- [ ] More better user experience
