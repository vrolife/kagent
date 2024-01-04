
add_library(insn STATIC insn.c)
target_include_directories(insn PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(insn PUBLIC kapi)
