cmake_minimum_required(VERSION 3.14)
project(myproj LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use local libs directory
set(LIBS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs")
set(HEADERS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/headers")

include_directories("${HEADERS_DIR}")
link_directories("${LIBS_DIR}")

# Set RPATH to find libraries in the libs directory
set(CMAKE_BUILD_RPATH "${LIBS_DIR}")
set(CMAKE_INSTALL_RPATH "${LIBS_DIR}")

find_package(SDL2 REQUIRED)

add_executable(myproj main.cpp headers/common-sdl.cpp)

# Link against whisper and ggml shared libraries
# Note: link order matters on Linux — dependencies first, dependents last
target_link_libraries(myproj PRIVATE whisper ggml-base ggml-cpu SDL2::SDL2 pthread dl)

# Optionally link CUDA backend if available
if(EXISTS "${LIBS_DIR}/libggml-cuda.so")
    target_link_libraries(myproj PRIVATE ggml-cuda)
endif()

target_include_directories(myproj PRIVATE ${SDL2_INCLUDE_DIR})
