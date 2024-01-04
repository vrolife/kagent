cmake_minimum_required(VERSION 3.12)
project(hello)

add_module(
    hello
    VERSION 0.1
    LICENSE GPL
    module.c
)
target_link_libraries(hello PRIVATE kapi)
