cmake_minimum_required(VERSION 3.25)
project(TOTP LANGUAGES CXX)

if(NOT WIN32 AND NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "TOTP supports Windows and Linux only.")
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

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/main.h")
file(STRINGS src/main.h TOTP_VERSION_LINE REGEX "^#define[ \t]+TOTP_PLUGIN_VER")
if(NOT TOTP_VERSION_LINE MATCHES "\"([0-9]+)[.]([0-9]+)[.]([0-9]+)\"")
    message(FATAL_ERROR "TOTP_PLUGIN_VER must be a numeric major.minor.patch version.")
endif()
set(TOTP_VERSION_MAJOR "${CMAKE_MATCH_1}")
set(TOTP_VERSION_MINOR "${CMAKE_MATCH_2}")
set(TOTP_VERSION_PATCH "${CMAKE_MATCH_3}")
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
target_link_libraries(TOTP PRIVATE OpenSSL::Crypto)

function(totp_configure_target target)
    target_compile_features(${target} PRIVATE cxx_std_17)
    target_include_directories(${target} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_BINARY_DIR}"
    )
    target_compile_definitions(${target} PRIVATE PAWN_CELL_SIZE=32)
    set_target_properties(${target} PROPERTIES
        CXX_EXTENSIONS OFF PREFIX "" OUTPUT_NAME TOTP
        POSITION_INDEPENDENT_CODE ON
        CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN YES
    )
    if(WIN32 AND MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
        set_target_properties(${target} PROPERTIES
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
        )
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    if(NOT WIN32)
        target_compile_definitions(${target} PRIVATE LINUX _TIME_BITS=64 _FILE_OFFSET_BITS=64)
        target_link_options(${target} PRIVATE -Wl,--no-undefined)
        set_target_properties(${target} PROPERTIES
            INSTALL_RPATH "$ORIGIN/.." INSTALL_RPATH_USE_LINK_PATH FALSE
        )
    endif()
endfunction()
totp_configure_target(TOTP)
if(WIN32)
    target_sources(TOTP PRIVATE TOTP.def)
    if(MSVC)
        set_source_files_properties(src/main.cpp PROPERTIES COMPILE_OPTIONS /Zc:strictStrings-)
    endif()
endif()

foreach(path include/sdk.hpp lib/glm/CMakeLists.txt lib/robin-hood-hashing/CMakeLists.txt
             lib/span-lite/CMakeLists.txt lib/string-view-lite/CMakeLists.txt)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lib/open.mp-sdk/${path}")
        message(FATAL_ERROR "Initialize the SDK: git submodule update --init --recursive")
    endif()
endforeach()
function(totp_add_openmp_sdk)
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
    set(CMAKE_SYSTEM_PROCESSOR x86)
    add_subdirectory(lib/open.mp-sdk EXCLUDE_FROM_ALL SYSTEM)
endfunction()
totp_add_openmp_sdk()
target_link_libraries(TOTP PRIVATE OMP-SDK)
target_include_directories(TOTP PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/lib/sdk")

install(TARGETS TOTP RUNTIME DESTINATION plugins LIBRARY DESTINATION plugins)
install(TARGETS TOTP RUNTIME DESTINATION components LIBRARY DESTINATION components)
install(FILES include/totp.inc DESTINATION qawno/include)
install(FILES include/totp.inc DESTINATION pawno/include)
install(FILES LICENSE README.md DESTINATION .)
