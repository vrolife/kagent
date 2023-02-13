cmake_minimum_required(VERSION 3.12)

if(ANDROID_PLATFORM)
    set(CMAKE_TOOLCHAIN_FILE $ENV{ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake)
endif()

project(kagent)

include(${CMAKE_CURRENT_LIST_DIR}/cmake/module.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/boost.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

set(CAPSTONE_ARM_SUPPORT OFF)
set(CAPSTONE_ARM64_SUPPORT OFF)
set(CAPSTONE_M68K_SUPPORT OFF)
set(CAPSTONE_MIPS_SUPPORT OFF)
set(CAPSTONE_PPC_SUPPORT OFF)
set(CAPSTONE_SPARC_SUPPORT OFF)
set(CAPSTONE_SYSZ_SUPPORT OFF)
set(CAPSTONE_XCORE_SUPPORT OFF)
set(CAPSTONE_X86_SUPPORT OFF)
set(CAPSTONE_TMS320C64X_SUPPORT OFF)
set(CAPSTONE_M680X_SUPPORT OFF)
set(CAPSTONE_EVM_SUPPORT OFF)

if (CMAKE_SYSTEM_PROCESSOR STREQUAL aarch64)
set(CAPSTONE_ARM64_SUPPORT ON)
endif()

if (CMAKE_SYSTEM_PROCESSOR STREQUAL arm)
set(CAPSTONE_ARM_SUPPORT ON)
endif()

if (CMAKE_SYSTEM_PROCESSOR STREQUAL x86_64)
set(CAPSTONE_X86_SUPPORT ON)
endif()

if (ANDROID_ABI STREQUAL arm64-v8a)
set(CAPSTONE_ARM64_SUPPORT ON)
endif()

if (ANDROID_ABI STREQUAL armeabi-v7a)
set(CAPSTONE_ARM_SUPPORT ON)
endif()

set(CAPSTONE_BUILD_STATIC ON)
set(CAPSTONE_BUILD_SHARED OFF)
set(CAPSTONE_BUILD_TESTS OFF)

add_subdirectory(capstone)
add_subdirectory(zlib)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/capstone/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_boost_libs(program_options log VERSION 1.81.0)

add_executable(kdeploy 
    kdeploy.cpp
    disasm.cpp
    utils.cpp
)

target_link_libraries(kdeploy PRIVATE 
    capstone-static zlibstatic 
    Boost::program_options
    Boost::log
)

add_subdirectory(modules)
