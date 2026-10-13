if(DEFINED ENV{VSCMD_ARG_TGT_ARCH} AND NOT "$ENV{VSCMD_ARG_TGT_ARCH}" STREQUAL "x86")
    message(FATAL_ERROR
        "The MSVC environment targets '$ENV{VSCMD_ARG_TGT_ARCH}', not x86. "
        "Run VsDevCmd -arch=x86 or vcvarsall x86 in a new shell.")
endif()

if(NOT CMAKE_CXX_COMPILER)
    find_program(TOTP_MSVC_COMPILER NAMES cl.exe)
    if(NOT TOTP_MSVC_COMPILER)
        message(FATAL_ERROR
            "MSVC cl.exe is missing from PATH. Install the x86 C++ Build Tools "
            "and initialize VsDevCmd -arch=x86 or vcvarsall x86, put Ninja on PATH.")
    endif()
    set(CMAKE_CXX_COMPILER "${TOTP_MSVC_COMPILER}")
endif()

include("${CMAKE_CURRENT_LIST_DIR}/check-x86-toolchain.cmake")
totp_check_x86_toolchain("${CMAKE_CXX_COMPILER}")
