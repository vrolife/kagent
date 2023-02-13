
include(ExternalProject)

function(add_module NAME)
    if(KAGENT_SOURCE_DIR)
        cmake_parse_arguments(MODULE "" "NAME;VERSION;LICENSE;AUTHOR;DESC;DEPENDS;SRCVERSION" "SOURCES" ${ARGN})
        
        if(NOT DEFINED MODULE_NAME)
            set(MODULE_NAME ${NAME})
        endif()

        include(ExternalProject)

        set(__TARGET_OPT --target=${MODULE_ARCH})
        
        set(__musl_CFLAGS "${CMAKE_C_FLAGS} ${__TARGET_OPT}")

        ExternalProject_Add(
            build_musl
            SOURCE_DIR ${KAGENT_SOURCE_DIR}
            CONFIGURE_COMMAND
                ${KAGENT_SOURCE_DIR}/musl/configure
                    --disable-shared
                    --prefix=${CMAKE_SYSROOT}
                    --build==${MODULE_TARGET}
                    --target=${MODULE_TARGET}
                    CC=${CMAKE_C_COMPILER}
                    CFLAGS=${__musl_CFLAGS}
                    AR=${CMAKE_AR}
                    RANLIB=${CMAKE_RANLIB}
            BUILD_COMMAND make -j
            INSTALL_COMMAND make install
        )

        configure_file(
            ${KAGENT_SOURCE_DIR}/mod.c.in
            ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.mod.c
            @ONLY
        )

        add_library(${NAME} MODULE 
            ${MODULE_SOURCES}
            ${MODULE_UNPARSED_ARGUMENTS}
            ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.mod.c
        )

        add_dependencies(${NAME} build_musl)

        set_target_properties(${NAME} PROPERTIES 
            PREFIX "" 
            SUFFIX ".ko"
            POSITION_INDEPENDENT_CODE OFF
        )

        # generate and compile symbol list
        add_custom_command(
            TARGET ${NAME} POST_BUILD
            COMMAND ${CMAKE_NM} -u $<TARGET_FILE:${NAME}> >${CMAKE_CURRENT_BINARY_DIR}/${NAME}.lst
            COMMAND
                python3 ${KAGENT_SOURCE_DIR}/scripts/generate_symbol_list.py
                    -o ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.lst.c
                    ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.lst
            COMMAND 
                ${CMAKE_C_COMPILER}
                    ${__CFLAGS} ${__TARGET_OPT}
                    -c
                    -o ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.lst.o
                    ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.lst.c
            COMMAND 
                ${CMAKE_COMMAND} -E rename $<TARGET_FILE:${NAME}> $<TARGET_FILE:${NAME}>.without-symbol-list
            COMMAND
                ${CMAKE_LINKER} 
                    ${__LDFLAGS}
                    -o $<TARGET_FILE:${NAME}>
                    ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.lst.o
                    $<TARGET_FILE:${NAME}>.without-symbol-list
        )
        
        install(
            TARGETS ${NAME}
            LIBRARY DESTINATION .
        )

    else(KAGENT_SOURCE_DIR)
        cmake_parse_arguments(MODULE "" "" "TARGET;CONFIGURE_ARGS" ${ARGN})

        if(NOT DEFINED MODULE_TARGET)
            if (CMAKE_C_COMPILER_TARGET)
                set(MODULE_TARGET ${CMAKE_C_COMPILER_TARGET})
            else()
                set(MODULE_TARGET ${CMAKE_SYSTEM_PROCESSOR}-linux-gnu)
            endif()
        endif()

        set(MODULE_C_COMPILER ${CMAKE_C_COMPILER})
        set(MODULE_CXX_COMPILER ${CMAKE_CXX_COMPILER})
        set(MODULE_LINKER ${CMAKE_LINKER})
        
        if (NOT DEFINED ANDROID_PLATFORM)
            set(MODULE_C_COMPILER clang)
            set(MODULE_CXX_COMPILER clang++)
            set(MODULE_LINKER ld.lld)
        endif()

        configure_file(
            ${CMAKE_SOURCE_DIR}/cmake/toolchain.cmake.in
            ${CMAKE_CURRENT_BINARY_DIR}/${MODULE_TARGET}-toolchain.cmake
            @ONLY
        )

        ExternalProject_Add(
            ${NAME}
            SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${NAME}
            BUILD_ALWAYS ON
            CMAKE_ARGS
                -DKAGENT_SOURCE_DIR=${CMAKE_SOURCE_DIR}
                -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/${NAME}
                -DCMAKE_TOOLCHAIN_FILE=${CMAKE_CURRENT_BINARY_DIR}/${MODULE_TARGET}-toolchain.cmake
                ${MODULE_CONFIGURE_ARGS}
            LOG_INSTALL OFF
        )
    endif(KAGENT_SOURCE_DIR)
endfunction()
