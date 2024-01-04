cmake_minimum_required(VERSION 3.12)
project(fingeradj)

add_module(
    fingeradj
    VERSION 0.1
    LICENSE GPL
    module.c
)
target_link_libraries(fingeradj PRIVATE kapi insn resolve_page)
