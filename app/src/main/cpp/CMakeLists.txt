cmake_minimum_required(VERSION 3.22.1)
project("stuja")

add_library(stuja SHARED
    native-lib.cpp
    MemoryScanner.cpp)

find_library(log-lib log)

target_link_libraries(stuja ${log-lib})
