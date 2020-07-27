cmake_minimum_required(VERSION 3.15.0)
project(robcoterm VERSION 0.1.0)

include(CTest)
enable_testing()

add_executable(robcoterm 
    source/main/main.cpp
    source/render/Console.cpp
    source/render/ConsoleSDLRenderer.cpp
    source/emulator/emulator.c
    source/main/syscall_handlers.cpp
    source/main/key_conversion.cpp
    source/assembler/assembler.cpp
    source/assembler/symbols.c
    )
target_include_directories(robcoterm PRIVATE 
    source/main
    source/render
    source/emulator
    source/assembler
    source/include)

if (APPLE)
    target_link_directories(robcoterm PUBLIC /usr/local/homebrew/lib)
    target_link_libraries(robcoterm stdc++ "-F /Library/Frameworks" "-framework SDL2" "-framework SDL2_image")
    find_path(SDL2_INCLUDE_DIR SDL2/SDL.h)
    find_path(SDL2_IMAGE_INCLUDE_DIR SDL2_image/SDL_image.h)
    target_include_directories(robcoterm PUBLIC ${SDL2_IMAGE_INCLUDE_DIR} ${SDL2_INCLUDE_DIR} /usr/local/homebrew/include)
    set (CMAKE_CXX_FLAGS "-std=c++17")
endif (APPLE)

if (MSVC)
    set_property(TARGET robcoterm PROPERTY VS_PACKAGE_REFERENCES "sdl2.nuget_2.0.5;sdl2_image.nuget_2.0.5")
endif ()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
