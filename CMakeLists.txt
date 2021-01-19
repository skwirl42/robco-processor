cmake_minimum_required(VERSION 3.15.0)
project(robcoterm VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)

find_package(SDL2 CONFIG REQUIRED)

if (MSVC)
set(BOOST_ROOT D:/boost_1_73_0)

find_package(sdl2-image CONFIG REQUIRED)

# Going to need to include specific boost libraries
#find_package(boost CONFIG REQUIRED)
endif()

find_package (Boost REQUIRED COMPONENTS program_options)

set(CONSOLE_DRAW_SOURCES
    source/console-draw/console_drawer.cpp
    source/console-draw/button.cpp
    source/console-draw/box.cpp
    source/console-draw/text_field.cpp
    source/console-draw/filesystem_viewer.cpp
)

set(ASSEMBLER_CORE_SOURCES
    source/main/opcode_table.c
    source/assembler/assembler.cpp
    source/assembler/symbols.c
    source/assembler/assembler_visitors.cpp
    source/assembler/assembler.hpp
    source/assembler/assembler_parser.hpp
    source/assembler/assembler_visitors.hpp
    source/assembler/parser_types.hpp
    source/assembler/symbols.h
    source/assembler/assembler_internal.hpp
    source/include/exceptions.hpp
    source/include/memory.h
    source/include/opcodes.h
)

add_executable(robcoterm
    ${CONSOLE_DRAW_SOURCES}
    ${ASSEMBLER_CORE_SOURCES}
    source/main/main.cpp
    source/render/Console.cpp
    source/render/ConsoleSDLRenderer.cpp
    source/emulator/emulator.c
    source/emulator/graphics.c
    source/emulator/holotape.c
    source/main/syscall_handlers.cpp
    source/main/syscall_holotape_handlers.cpp
    source/main/key_conversion.cpp
    source/sound/sound_system.cpp
    source/include/program_options_helpers.hpp
    )

target_include_directories(robcoterm PRIVATE 
    source/main
    source/render
    source/emulator
    source/assembler
    source/include
    source/sound
    source/console-draw
    ${Boost_INCLUDE_DIRS})

add_executable(assembler
    ${ASSEMBLER_CORE_SOURCES}
    source/assembler/assembler_main.cpp
    source/include/program_options_helpers.hpp
    )

target_include_directories(assembler PRIVATE 
    source/main
    source/assembler
    source/include
    ${Boost_INCLUDE_DIRS})

add_executable(tapemanager
    source/tapemanager/tapemanager_main.cpp
    source/tapemanager/holotape_wrapper.cpp
    source/include/program_options_helpers.hpp
    source/emulator/holotape.c
)

target_include_directories(tapemanager PRIVATE 
    source/main
    source/tapemanager
    source/include
    source/emulator
    source/assembler
    ${Boost_INCLUDE_DIRS})

add_executable(sound_test
    source/sound/sound_test_main.cpp
    source/sound/sound_system.cpp)

target_include_directories(sound_test PRIVATE
    source/include
    ${Boost_INCLUDE_DIRS})

add_executable(sound_keyboard
    source/sound/sound_keyboard_main.cpp
    source/sound/sound_system.cpp)

target_include_directories(sound_keyboard PRIVATE
    source/include
    ${Boost_INCLUDE_DIRS})

target_link_directories(robcoterm PUBLIC ${Boost_LIBRARY_DIRS})
target_link_directories(assembler PUBLIC ${Boost_LIBRARY_DIRS})
target_link_directories(tapemanager PUBLIC ${Boost_LIBRARY_DIRS})
target_link_directories(sound_test PUBLIC ${Boost_LIBRARY_DIRS})
target_link_directories(sound_keyboard PUBLIC ${Boost_LIBRARY_DIRS})

if (APPLE)
    target_link_directories(assembler PUBLIC /opt/homebrew/lib)
    target_link_libraries(assembler stdc++ ${Boost_LIBRARIES})
    target_include_directories(assembler PUBLIC /opt/homebrew/include)

    target_link_directories(robcoterm PUBLIC /opt/homebrew/lib)
    target_link_libraries(robcoterm stdc++ "-lSDL2" "-lSDL2_image" ${Boost_LIBRARIES})
    target_include_directories(robcoterm PUBLIC /opt/homebrew/include)

    target_link_libraries(tapemanager stdc++ ${Boost_LIBRARIES})

    target_link_directories(sound_test PUBLIC /opt/homebrew/lib)
    target_link_libraries(sound_test stdc++ "-lSDL2" "-lSDL2_image" ${Boost_LIBRARIES})
    target_include_directories(sound_test PUBLIC ${SDL2_INCLUDE_DIR} /opt/homebrew/include)

    target_link_directories(sound_keyboard PUBLIC /opt/homebrew/lib)
    target_link_libraries(sound_keyboard stdc++ "-lSDL2" "-lSDL2_image" ${Boost_LIBRARIES})
    target_include_directories(sound_keyboard PUBLIC ${SDL2_INCLUDE_DIR} /opt/homebrew/include)

    set (CMAKE_CXX_FLAGS "-std=c++17 -DAPPLE")
endif (APPLE)

if (MSVC)
    target_include_directories(robcoterm PUBLIC D:/GnuWin32/include)
    target_link_libraries(robcoterm PRIVATE SDL2::SDL2 SDL2::SDL2main SDL2::SDL2_image ${Boost_LIBRARIES})

    target_include_directories(assembler PUBLIC D:/GnuWin32/include)
    target_link_libraries(assembler PRIVATE ${Boost_LIBRARIES})

    target_include_directories(tapemanager PUBLIC D:/GnuWin32/include)
    target_link_libraries(tapemanager PRIVATE ${Boost_LIBRARIES})

    target_link_libraries(sound_test PRIVATE SDL2::SDL2 SDL2::SDL2main ${Boost_LIBRARIES})

    target_link_libraries(sound_keyboard PRIVATE SDL2::SDL2 SDL2::SDL2main SDL2::SDL2_image ${Boost_LIBRARIES})
endif ()
