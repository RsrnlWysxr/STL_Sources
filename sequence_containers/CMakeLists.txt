cmake_minimum_required(VERSION 3.12)
project(sequence_containers)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(sequence_containers
        cmake-build-debug/cmake_install.cmake
        cmake-build-debug/CMakeCache.txt
        cmake-build-debug/Makefile
        cmake-build-debug/sequence_containers
        cmake-build-debug/sequence_containers.cbp
        CMakeLists.txt
        JJ.h
        LYH.h
        main.cpp
        sequence_containers.h)
