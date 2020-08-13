cmake_minimum_required(VERSION 3.15.0)
project(robcoterm VERSION 0.1.0)

include(CTest)
enable_testing()

if (APPLE) # The built-in flex on macOS doesn't play nice with C++17
set(FLEX_ROOT /usr/local/homebrew/opt/flex)
set(FLEX_EXECUTABLE /usr/local/homebrew/opt/flex/bin/flex)
set(BISON_ROOT /usr/local/homebrew/opt/bison@2.7)
set(BISON_EXECUTABLE /usr/local/homebrew/opt/bison@2.7/bin/bison)
endif ()

if (MSVC) # Wanted to use WSL, but it was causing issues - cygwin instead?
set(FLEX_EXECUTABLE D:/GnuWin32/bin/flex)
set(BISON_EXECUTABLE D:/GnuWin32/bin/bison)
endif()

find_package (BISON)

find_package (FLEX)

BISON_TARGET(AssemblerYacc source/assembler/assembler.y ${CMAKE_CURRENT_BINARY_DIR}/assembler_yacc.cpp)
FLEX_TARGET(AssemblerLex source/assembler/assembler.l  ${CMAKE_CURRENT_BINARY_DIR}/assembler_lex.cpp)

BISON_TARGET(CompilerYacc source/compiler/compiler.y ${CMAKE_CURRENT_BINARY_DIR}/compiler_yacc.cpp)
FLEX_TARGET(CompilerLex source/compiler/compiler.l  ${CMAKE_CURRENT_BINARY_DIR}/compiler_lex.cpp)

add_executable(robcoterm 
    source/main/main.cpp
    source/render/Console.cpp
    source/render/ConsoleSDLRenderer.cpp
    source/emulator/emulator.c
    source/emulator/graphics.c
    source/main/syscall_handlers.cpp
    source/main/key_conversion.cpp
    source/main/opcode_table.c
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

find_package (Boost COMPONENTS wave)

add_executable(compiler
    source/compiler/compiler_main.cpp
    source/compiler/compiler.cpp
    source/main/opcode_table.c
    source/assembler/symbols.c
    ${FLEX_CompilerLex_OUTPUTS}
    ${BISON_CompilerYacc_OUTPUTS}
    )

target_include_directories(compiler PRIVATE 
    source/main
    source/compiler
    source/assembler
    source/include
    ${Boost_INCLUDE_DIRS})

if (APPLE)
    add_executable(assembler
        source/main/opcode_table.c
        source/assembler/assembler.cpp
        source/assembler/assembler_main.cpp
        source/assembler/symbols.c
        ${FLEX_AssemblerLex_OUTPUTS}
        ${BISON_AssemblerYacc_OUTPUTS}
        )

    target_include_directories(assembler PRIVATE 
        source/main
        source/assembler
        source/include)

    target_link_directories(assembler PUBLIC /usr/local/homebrew/lib)
    target_link_libraries(assembler stdc++)
    target_include_directories(assembler PUBLIC /usr/local/homebrew/include)

    target_link_directories(compiler PUBLIC /usr/local/homebrew/lib ${Boost_LIBRARY_DIRS})
    target_link_libraries(compiler stdc++ ${Boost_LIBRARIES})
    target_include_directories(compiler PUBLIC /usr/local/homebrew/include)

    target_link_directories(robcoterm PUBLIC /usr/local/homebrew/lib)
    target_link_libraries(robcoterm stdc++ "-F /Library/Frameworks" "-framework SDL2" "-framework SDL2_image")
    find_path(SDL2_INCLUDE_DIR SDL2/SDL.h)
    find_path(SDL2_IMAGE_INCLUDE_DIR SDL2_image/SDL_image.h)
    target_include_directories(robcoterm PUBLIC ${SDL2_IMAGE_INCLUDE_DIR} ${SDL2_INCLUDE_DIR} /usr/local/homebrew/include)
    set (CMAKE_CXX_FLAGS "-std=c++17 -DAPPLE")
endif (APPLE)

if (MSVC)
    target_include_directories(robcoterm PUBLIC D:/GnuWin32/include)
    target_include_directories(compiler PUBLIC D:/GnuWin32/include)
endif ()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
