cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(geiger)

target_sources(app PRIVATE 
    src/main.c
    #src/uartRx.h
    #src/uartRx.c
    src/fram.h
    src/fram.c
)
