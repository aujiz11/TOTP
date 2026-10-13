set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR i686)

if(NOT CMAKE_CXX_COMPILER)
    find_program(TOTP_GCC_COMPILER NAMES g++)
    if(NOT TOTP_GCC_COMPILER)
        message(FATAL_ERROR
            "g++ is missing from PATH. Install GCC multilib and 32-bit C/C++ "
            "development/runtime libraries; put Ninja on PATH.")
    endif()
    set(CMAKE_CXX_COMPILER "${TOTP_GCC_COMPILER};-m32")
endif()

set(_totp_compiler_command ${CMAKE_CXX_COMPILER})
list(GET _totp_compiler_command 0 _totp_compiler)
include("${CMAKE_CURRENT_LIST_DIR}/check-x86-toolchain.cmake")
totp_check_x86_toolchain("${_totp_compiler}" -m32)
