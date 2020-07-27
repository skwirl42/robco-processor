# robco-processor
> an emulated fantasy computer system inspired by the terminals found in the Fallout universe

A bit of an expansion on the ideas in https://github.com/skwirl42/robco-os including an ISA for a stack-based processor.

Platforms:
- macOS 10.15+ with dev tools installed (could probably be easily ported to other platforms/versions)
- Windows with Visual Studio installed
  * note: you'll need to add SDL2 and SLD2_image using nuget, as cmake's nuget support and VS C++ projects don't get along

Requirements:
- CMake 3.15+
- SDL2
- SDL2_image
