cmake_minimum_required(VERSION 3.15.0)
project(robcoterm VERSION 0.1.0)

include(CTest)
enable_testing()

find_package (BISON)

if (APPLE) # The built-in flex on macOS doesn't play nice with C++17
set(FLEX_ROOT /usr/local/homebrew/opt/flex)
set(FLEX_EXECUTABLE /usr/local/homebrew/opt/flex/bin/flex)
endif ()

find_package (FLEX)

BISON_TARGET(AssemblerYacc source/assembler/assembler.y ${CMAKE_CURRENT_BINARY_DIR}/assembler_yacc.cpp)
FLEX_TARGET(AssemblerLex source/assembler/assembler.l  ${CMAKE_CURRENT_BINARY_DIR}/assembler_lex.cpp)

add_executable(robcoterm 
    source/main/main.cpp
    source/render/Console.cpp
    source/render/ConsoleSDLRenderer.cpp
    source/emulator/emulator.c
    source/main/syscall_handlers.cpp
    source/main/key_conversion.cpp
    source/assembler/assembler.cpp
    source/assembler/symbols.c
    ${FLEX_AssemblerLex_OUTPUTS}
    ${BISON_AssemblerYacc_OUTPUTS}
    )
target_include_directories(robcoterm PRIVATE 
    source/main
    source/render
    source/emulator
    source/assembler
    source/include)

FLEX_TARGET(AssemblerLex source/assembler/assembler.l  ${CMAKE_CURRENT_BINARY_DIR}/assembler_lex.cpp)

if (APPLE)
    target_link_directories(robcoterm PUBLIC /usr/local/homebrew/lib)
    target_link_libraries(robcoterm stdc++ "-F /Library/Frameworks" "-framework SDL2" "-framework SDL2_image")
    find_path(SDL2_INCLUDE_DIR SDL2/SDL.h)
    find_path(SDL2_IMAGE_INCLUDE_DIR SDL2_image/SDL_image.h)
    target_include_directories(robcoterm PUBLIC ${SDL2_IMAGE_INCLUDE_DIR} ${SDL2_INCLUDE_DIR} /usr/local/homebrew/include)
    set (CMAKE_CXX_FLAGS "-std=c++17 -DAPPLE")
endif (APPLE)

if (MSVC)
    set_property(TARGET robcoterm PROPERTY VS_PACKAGE_REFERENCES "sdl2.nuget_2.0.5;sdl2_image.nuget_2.0.5")
endif ()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
