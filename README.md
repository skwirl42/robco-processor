# robco-processor
> an emulated fantasy computer system inspired by the terminals found in the Fallout universe

A bit of an expansion on the ideas in https://github.com/skwirl42/robco-os including an ISA for a stack-based processor.

## Platforms:
- macOS 10.15+ with dev tools installed
  * Install SDL2 and SLD2_image to /Library/Frameworks in any way you choose
  * flex (latest) and bison (2.7) need to be installed via homebrew: https://brew.sh/
- Windows with Visual Studio installed
  * note: you'll need to add boost, SDL2 and SLD2_image using vcpkg
  * flex and bison should be installed from the GnuWin32 site http://gnuwin32.sourceforge.net/
- Could probably be easily ported to other platforms/versions, as code is straight C/C++ and SDL2

## Requirements:
- CMake 3.15+
- C++17 compiler
- SDL2
- SDL2_image
- flex
- bison
- boost

## The Debugger
While running the emulator clicking in the window with the mouse will pause execution and bring up the debugging screen. It shows the status of the registers, the contents of the stack, the data pointed at by the index registers, and the instruction that last executed. To resume normal execution press F5.

## The Assembler
While the main executable assembles a program before executing it, you can use the "assembler" cmake target to make a standalone version of the assembler. The standalone assembler outputs a text file containing the hexadecimal code and data regions, and a list of symbols defined in the program. This file is not meant to be executed, but rather for debugging and testing purposes.

## Mix of languages
When I started this I thought some pieces should be in C++ and others in C. This was to keep the parts that needed better performance as C, but I'm not sure it's really necessary. I won't be rewriting the parts in C, but I plan to continue any new development in C++.

## TODO
### Ease of use
- [ ] Write a programming guide
- [ ] Implement a higher level language (probably BASIC, at first)
- [ ] More better user experience
- [ ] Add a configuration screen for inserting/ejecting holotapes, setting emulator parameters, etc
## Functionality
- [ ] Implement audio system using https://github.com/OneLoneCoder/synth ported to SDL, or something similar
  - implementation done, needs extensive testing
### Refactoring/increased cross-platform parity
- [ ] Rewrite assembler parsing using boost::spirit
### Complete
- [x] Implement support for executing programs from tape
- [x] Rewrite command line options using boost's Program Options library
- [x] Better error handling for the assembler
- [x] Update the debugger to support showing the next instruction to be executed, instead of the last
- [x] Update the debugger to properly show the source arguments for register indexed instructions
- [x] Allow for swapping between the debug and regular console while execution remains paused (hold right mouse button)
