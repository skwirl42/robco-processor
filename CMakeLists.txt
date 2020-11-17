cmake_minimum_required(VERSION 3.15.0)
project(robcoterm VERSION 0.1.0)

include(CTest)
enable_testing()

set(CMAKE_CXX_STANDARD 17)

if (APPLE) # The built-in flex on macOS doesn't play nice with C++17
set(FLEX_ROOT /usr/local/homebrew/opt/flex)
set(FLEX_EXECUTABLE /usr/local/homebrew/opt/flex/bin/flex)
set(BISON_ROOT /usr/local/homebrew/opt/bison@2.7)
set(BISON_EXECUTABLE /usr/local/homebrew/opt/bison@2.7/bin/bison)
endif ()

if (MSVC) # Wanted to use WSL, but it was causing issues - GnuWin32
set(FLEX_EXECUTABLE D:/GnuWin32/bin/flex)
set(BISON_EXECUTABLE D:/GnuWin32/bin/bison)
set(BOOST_ROOT D:/boost_1_73_0)
endif()

find_package(Boost)
find_package (BISON)
find_package (FLEX)

BISON_TARGET(AssemblerYacc source/assembler/assembler.y ${CMAKE_CURRENT_BINARY_DIR}/assembler_yacc.cpp)
FLEX_TARGET(AssemblerLex source/assembler/assembler.l  ${CMAKE_CURRENT_BINARY_DIR}/assembler_lex.cpp)

add_executable(robcoterm 
    source/main/main.cpp
    source/render/Console.cpp
    source/render/ConsoleSDLRenderer.cpp
    source/emulator/emulator.c
    source/emulator/graphics.c
    source/emulator/holotape.c
    source/main/syscall_handlers.cpp
    source/main/syscall_holotape_handlers.cpp
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
    source/include
    ${Boost_INCLUDE_DIRS})

add_executable(assembler
    source/main/opcode_table.c
    source/assembler/assembler.cpp
    source/assembler/assembler_main.cpp
    source/assembler/symbols.c
    ${FLEX_AssemblerLex_OUTPUTS}
    ${BISON_AssemblerYacc_OUTPUTS})

target_include_directories(assembler PRIVATE 
    source/main
    source/assembler
    source/include
    ${Boost_INCLUDE_DIRS})

add_executable(tapemanager
    source/tapemanager/tapemanager_main.cpp
    source/tapemanager/holotape_wrapper.cpp
    source/emulator/holotape.c)

target_include_directories(tapemanager PRIVATE 
    source/main
    source/tapemanager
    source/include
    source/emulator
    source/assembler
    ${Boost_INCLUDE_DIRS})

target_link_directories(robcoterm PUBLIC ${Boost_LIBRARY_DIRS})
target_link_directories(assembler PUBLIC ${Boost_LIBRARY_DIRS})
target_link_directories(tapemanager PUBLIC ${Boost_LIBRARY_DIRS})

if (APPLE)
    target_link_directories(assembler PUBLIC /usr/local/homebrew/lib)
    target_link_libraries(assembler stdc++)
    target_include_directories(assembler PUBLIC /usr/local/homebrew/include)

    target_link_directories(robcoterm PUBLIC /usr/local/homebrew/lib)
    target_link_libraries(robcoterm stdc++ "-F /Library/Frameworks" "-framework SDL2" "-framework SDL2_image")
    find_path(SDL2_INCLUDE_DIR SDL2/SDL.h)
    find_path(SDL2_IMAGE_INCLUDE_DIR SDL2_image/SDL_image.h)
    target_include_directories(robcoterm PUBLIC ${SDL2_IMAGE_INCLUDE_DIR} ${SDL2_INCLUDE_DIR} /usr/local/homebrew/include)
    set (CMAKE_CXX_FLAGS "-std=c++17 -DAPPLE")
endif (APPLE)

if (MSVC)
    target_include_directories(robcoterm PUBLIC D:/GnuWin32/include)
    target_include_directories(assembler PUBLIC D:/GnuWin32/include source/xgetopt)
    target_sources(tapemanager PUBLIC source/xgetopt/XGetopt.cpp)
    target_include_directories(tapemanager PUBLIC D:/GnuWin32/include source/xgetopt)
    target_sources(tapemanager PUBLIC source/xgetopt/XGetopt.cpp)
endif ()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
