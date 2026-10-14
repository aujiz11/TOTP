// Base32 implementation
//
// Copyright 2010 Google Inc.
// Author: Markus Gutschke
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// RFC 4648 Base32. Legacy normalization is retained by base32_decode.
// No silent truncation. Failed decoding clears written bytes and reports zero length.
// Input/output spans must be valid, separate allocations, overlap is unsupported.

#ifndef TOTP_BASE32_H
#define TOTP_BASE32_H

#include <cstddef>
#include <stdint.h>

namespace base32 {
    constexpr std::size_t max_encoded_bytes = 160;
    constexpr std::size_t max_decoded_bytes = 100;

    enum class Mode { strict, legacy };
    enum class Status { ok, invalid_input, output_too_small };
    struct DecodeResult { Status status; std::size_t length; };

    DecodeResult decode(const uint8_t* input, std::size_t length, uint8_t* output,
                        std::size_t capacity, Mode mode);
}

int base32_decode(const uint8_t* encoded, uint8_t* result, int bufSize);
int base32_encode(const uint8_t* data, int length, uint8_t* result, int bufSize);

#endif
