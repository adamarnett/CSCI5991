cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test0)

target_sources(app PRIVATE 
    src/i2c_wrapper.h
    src/main.c
)