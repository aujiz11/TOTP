cmake_minimum_required(VERSION 3.25)
project(TOTP LANGUAGES CXX)

if(NOT WIN32 AND NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "The legacy TOTP plugin supports Windows and Linux only.")
endif()
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "TOTP requires an x86 toolchain with 32-bit pointers.")
endif()

include(CheckCXXSourceCompiles)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/cmake/x86-probe.cpp" TOTP_X86_PROBE_SOURCE)
check_cxx_source_compiles("${TOTP_X86_PROBE_SOURCE}" TOTP_COMPILER_TARGETS_X86)
if(NOT TOTP_COMPILER_TARGETS_X86)
    message(FATAL_ERROR
        "TOTP requires a working x86 C++ compile/link environment with 32-bit "
        "runtime libraries. Initialize MSVC x86 tools or install Linux GCC multilib.")
endif()

find_package(OpenSSL 3.5.9 EXACT REQUIRED COMPONENTS Crypto)

math(EXPR TOTP_BUILD_POINTER_BITS "${CMAKE_SIZEOF_VOID_P} * 8")
configure_file(cmake/totp_build_config.h.in generated/totp_build_config.h @ONLY)

file(GLOB_RECURSE TOTP_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*"
)

add_library(TOTP SHARED
    ${TOTP_SOURCES}
    lib/sdk/amxplugin.cpp
    "${CMAKE_CURRENT_BINARY_DIR}/generated/totp_build_config.h"
)
target_compile_features(TOTP PRIVATE cxx_std_17)
target_link_libraries(TOTP PRIVATE OpenSSL::Crypto)
target_include_directories(TOTP PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_compile_definitions(TOTP PRIVATE PAWN_CELL_SIZE=32)
set_target_properties(TOTP PROPERTIES
    CXX_EXTENSIONS OFF
    OUTPUT_NAME "TOTP"
    PREFIX ""
    POSITION_INDEPENDENT_CODE ON
)

if(WIN32)
    target_sources(TOTP PRIVATE TOTP.def)
    if(MSVC)
        target_compile_options(TOTP PRIVATE /W4 /permissive-)
        set_source_files_properties(src/main.cpp PROPERTIES COMPILE_OPTIONS /Zc:strictStrings-)
        set_target_properties(TOTP PROPERTIES
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
        )
    else()
        target_compile_options(TOTP PRIVATE -Wall -Wextra -Wpedantic)
    endif()
else()
    target_compile_definitions(TOTP PRIVATE LINUX _TIME_BITS=64 _FILE_OFFSET_BITS=64)
    target_compile_options(TOTP PRIVATE -Wall -Wextra -Wpedantic)
    target_link_options(TOTP PRIVATE -Wl,--no-undefined)
    set_target_properties(TOTP PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES
    )
endif()
