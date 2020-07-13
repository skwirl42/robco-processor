cmake_minimum_required(VERSION 3.0.0)
project(robcoterm VERSION 0.1.0)

include(CTest)
enable_testing()

add_executable(robcoterm source/main/main.cpp source/render/Console.cpp source/render/ConsoleSDLRenderer.cpp)
target_include_directories(robcoterm PRIVATE source/main source/render)

if (APPLE)
    target_link_libraries(robcoterm stdc++ "-F /Library/Frameworks" "-framework SDL2" "-framework SDL2_image")
    find_path(SDL2_INCLUDE_DIR SDL2/SDL.h)
    find_path(SDL2_IMAGE_INCLUDE_DIR SDL2_image/SDL_image.h)
    target_include_directories(robcoterm PUBLIC ${SDL2_IMAGE_INCLUDE_DIR} ${SDL2_INCLUDE_DIR})
endif (APPLE)

set (CMAKE_CXX_FLAGS "-std=c++17")

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
