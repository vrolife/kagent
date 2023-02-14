
add_executable(test_vmrw test_vmrw.cpp ${CMAKE_SOURCE_DIR}/modules/vmrw/client.c)
target_include_directories(test_vmrw PRIVATE ${CMAKE_SOURCE_DIR}/modules/vmrw)
