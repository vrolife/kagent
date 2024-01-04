cmake_minimum_required(VERSION 3.12)
project(vmrw)

add_module(
    vmrw
    VERSION 0.1
    LICENSE GPL
    module.c
)
target_link_libraries(vmrw PRIVATE kapi resolve_page)
