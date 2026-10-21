TOTP for SA:MP/open.mp
=====================

See [original version](https://github.com/GamesDV/TOTP)

This plugin allows you to generate one-time passwords based on time (and computing on algorithm based TOTP RFC 6238) are also used by Google in the Google Authenticator.

SA:MP uses the legacy plugin in `plugins/`. open.mp uses the native runtime
component in `components/`. Both provide the same five Pawn natives below.

New generation/verification uses the server's 64-bit UTC clock, HMAC-SHA1,
30-second steps and strict Base32 secrets. Verification accepts `skew` from
0 to 2, counting adjacent steps on each side, the default is 1. The legacy
native keeps its signed 32-bit UTC timestamp argument and permissive secret
normalization. Its numeric result does not preserve leading zeros.

Verification is stateless: replay prevention and login rate limits remain
the account service's responsibility.

Installation
------------

The same binary exports the legacy plugin entry points and open.mp's
`ComponentEntryPoint`. The entry point called by the server selects the runtime;
there is no separate component build or runtime flag to configure.

Extract the ZIP into your server root, preserving paths. Keep `libcrypto-3.dll`
(Windows) or `libcrypto.so.3` (Linux) beside the server executable. Windows
also needs the installed x86 Microsoft Visual C++ Redistributable.

### SA:MP

Use `plugins/TOTP.dll` on Windows or `plugins/TOTP.so` on Linux. Append `TOTP`
(Windows) or `TOTP.so` (Linux) to the existing `plugins` line in `server.cfg`.

### open.mp

Use the same binary as `components/TOTP.dll` on Windows or `components/TOTP.so`
on Linux. Keep the Pawn component enabled. Remove TOTP from `pawn.legacy_plugins`
and any legacy `plugins` setting. Load one TOTP copy per server.

Include `totp.inc` from your `pawno/include/` or `qawno/include/`.
Existing Pawn scripts use the same native signatures on both servers.

Usage
--------------


### GoogleAuthenticatorCode

```pawn
native GoogleAuthenticatorCode(const secret[], timestamp);
```

**Params**

- `secret`: Base32 secret using legacy normalization, including lowercase and supported separators.
- `timestamp`: UTC Unix seconds in a signed 32-bit Pawn cell

**Description**

Returns a numeric six-digit code from `0` to `999999`, or `-1` on error.
Leading zeros are lost in the numeric result. This legacy signature remains
available for existing scripts.

### TOTP_GenerateCode

```pawn
native TOTP_GenerateCode(const secret[], code[], code_size);
```

**Params**

- `secret`: Strict uppercase Base32 secret, at most `TOTP_MAX_SECRET_LENGTH` (160) characters. Canonical terminal padding is accepted.
- `code`: Unpacked output array for the six-digit string and NUL terminator.
- `code_size`: Output capacity in cells, at least `TOTP_CODE_SIZE` (7).

**Description**

Generates the current code using server UTC time and preserves leading zeros.
Returns `TOTP_SUCCESS` on success or `TOTP_ERROR` on invalid input, insufficient
output capacity, clock or backend failure. Input and output arrays must not overlap.

### TOTP_VerifyCode

```pawn
native TOTP_VerifyCode(const secret[], const code[], skew = TOTP_DEFAULT_SKEW);
```

**Params**

- `secret`: Strict uppercase Base32 secret, with the same rules as `TOTP_GenerateCode`.
- `code`: Exactly six ASCII digits, including any leading zeros.
- `skew`: Adjacent 30-second steps accepted on each side. Defaults to `TOTP_DEFAULT_SKEW` (1), valid range is 0 through `TOTP_MAX_SKEW` (2).

**Description**

Returns `TOTP_SUCCESS` for a match, `TOTP_MISMATCH` for a well-formed code that
does not match, or `TOTP_ERROR` for malformed input, invalid skew, clock or
backend failure. With `skew = 1`, checks the previous, current and next step.


### TOTP_GenerateSecret

```pawn
native TOTP_GenerateSecret(secret[], secret_size);
```

**Params**

- `secret`: Unpacked output array for the generated Base32 secret.
- `secret_size`: Output capacity in cells, at least `TOTP_SECRET_SIZE` (33).

**Description**

Generates 160 random bits using the cryptographic backend and encodes them as
32 uppercase Base32 characters without padding. Returns `TOTP_SUCCESS` or
`TOTP_ERROR`, random-generator failure produces no usable secret.

### TOTP_GetVersion

```pawn
native TOTP_GetVersion(version[], version_size);
```

**Params**

- `version`: Unpacked output array for the plugin's runtime version string.
- `version_size`: Output capacity in cells, at least `TOTP_VERSION_SIZE` (32).

**Description**

Writes the version displayed in the plugin's startup banner. Returns
`TOTP_SUCCESS` or `TOTP_ERROR`. Version output is never truncated.

Build
-----

Build from the repository root using CMake 3.25+, Ninja and a C++17 compiler.

```sh
git submodule update --init --recursive
```

### Windows

```bat
cmake --preset windows-x86-release
cmake --build --preset windows-x86-release
```

Outputs: `build/windows-x86-release/TOTP.dll`

### Linux

Install GCC multilib, 32-bit C/C++ development libraries, CMake, Ninja,
Perl and the usual build tools. On Debian/Ubuntu, compiler dependencies
include `g++-multilib` and `libc6-dev-i386`.
The plugin requires a signed 64-bit `time_t`. CMake enables `_TIME_BITS=64`
and `_FILE_OFFSET_BITS=64`. For 32-bit glibc builds this needs glibc 2.34+.
Post-2038 system time also requires kernel support for 64-bit time syscalls.

```sh
cmake --preset linux-x86-release
cmake --build --preset linux-x86-release
```

Outputs: `build/linux-x86-release/TOTP.so`
