# robco-processor
> an emulated fantasy computer system inspired by the terminals found in the Fallout universe

A bit of an expansion on the ideas in https://github.com/skwirl42/robco-os including an ISA for a stack-based processor.

## Platforms:
- macOS 10.15+ with dev tools installed
  * SDL2, SDL2_image, and boost are easily installed using brew.sh
- Windows with Visual Studio installed
  * note: you'll need to add boost, SDL2 and SLD2_image using vcpkg
- Could probably be easily ported to other platforms/versions, as code is straight C/C++ and SDL2

## Requirements:
- CMake 3.15+
- C++17 compiler
- SDL2
- SDL2_image
- boost

## Blog posts

I've got a blog post at https://ideaoubliette.blogspot.com/2021/10/emulator-basics-running-code.html on running code in the emulator.

## The Debugger
While running the emulator clicking in the window with the mouse will pause execution and bring up the debugging screen. It shows the status of the registers, the contents of the stack, the data pointed at by the index registers, and the instruction that last executed. To resume normal execution press F5.

## The Assembler
While the main executable assembles a program before executing it, you can use the "assembler" cmake target to make a standalone version of the assembler. The standalone assembler outputs a text file containing the hexadecimal code and data regions, and a list of symbols defined in the program. This file is not meant to be executed, but rather for debugging and testing purposes.

## Mix of languages
When I started this I thought some pieces should be in C++ and others in C. This was to keep the parts that needed better performance as C, but I'm not sure it's really necessary. I won't be rewriting the parts in C, but I plan to continue any new development in C++.

## TODO
### Moved
- Most TODOs have moved to the project boards https://github.com/skwirl42/robco-processor/projects
## Functionality
- [ ] Implement audio system using https://github.com/OneLoneCoder/synth ported to SDL, or something similar
  - implementation done, needs extensive testing
