function(totp_check_x86_toolchain compiler)
    get_property(_in_try_compile GLOBAL PROPERTY IN_TRY_COMPILE)
    get_property(_already_checked GLOBAL PROPERTY TOTP_X86_PREFLIGHT_COMPLETE)
    if(_in_try_compile OR _already_checked)
        return()
    endif()

    set(_check_dir "${CMAKE_BINARY_DIR}/CMakeFiles/totp-x86-check")
    file(MAKE_DIRECTORY "${_check_dir}")
    set(_probe "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/x86-probe.cpp")
    if(CMAKE_HOST_WIN32)
        execute_process(
            COMMAND "${compiler}" /nologo /EHsc /std:c++17 /MD
                "${_probe}" "/Fo${_check_dir}/probe.obj" "/Fe${_check_dir}/probe.exe"
                /link /MACHINE:X86
            WORKING_DIRECTORY "${_check_dir}"
            RESULT_VARIABLE _result OUTPUT_VARIABLE _stdout ERROR_VARIABLE _stderr
            TIMEOUT 30
        )
        set(_help "Initialize an MSVC x86 environment (VsDevCmd -arch=x86 or vcvarsall x86) with a Windows SDK and x86 C++ runtime libraries.")
    else()
        execute_process(
            COMMAND "${compiler}" ${ARGN} -std=c++17 "${_probe}" -o "${_check_dir}/probe"
            WORKING_DIRECTORY "${_check_dir}"
            RESULT_VARIABLE _result OUTPUT_VARIABLE _stdout ERROR_VARIABLE _stderr
            TIMEOUT 30
        )
        set(_help "Install GCC multilib and 32-bit C/C++ development/runtime libraries for your Linux distribution.")
    endif()

    if(NOT "${_result}" STREQUAL "0")
        message(FATAL_ERROR
            "The x86 C++17 compile AND link check failed. ${_help}\n"
            "Compiler: ${compiler}\nResult: ${_result}\n${_stdout}${_stderr}")
    endif()
    set_property(GLOBAL PROPERTY TOTP_X86_PREFLIGHT_COMPLETE TRUE)
    message(STATUS "TOTP x86 C++17 compile and link preflight passed")
endfunction()
