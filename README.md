TOTP plugin for SA:MP
=====================

See [original version](https://github.com/GamesDV/TOTP)

This plugin allows you to generate one-time passwords based on time (and computing on algorithm based TOTP RFC 6238) are also used by Google in the Google Authenticator.

Build
-----

Build from the repository root using CMake 3.25+, Ninja and a C++17 compiler.

### Windows

```sh
cmake -S . -B build/windows-x86-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/windows-x86-release
```

Output: `build/windows-x86-release/TOTP.dll`.

### Linux

For Debian/Ubuntu, the compiler dependencies typically include
`g++-multilib` and `libc6-dev-i386`.

```sh
cmake -S . -B build/linux-x86-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS=-m32
cmake --build build/linux-x86-release
```

Output: `build/linux-x86-release/TOTP.so`.


For Debug, use a separate build directory and `-DCMAKE_BUILD_TYPE=Debug`.
Windows Debug/Release x86 builds have been verified. Use Release for deployment.
