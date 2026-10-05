cmake_minimum_required(VERSION 3.10.2)
project(base64_package_test C)

find_package(base64 CONFIG REQUIRED)

add_executable(test_package main.c)
target_link_libraries(test_package PRIVATE aklomp::base64)

enable_testing()
add_test(NAME test_package COMMAND test_package)
