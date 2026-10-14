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

#include "base32.h"

namespace {
    void erase(uint8_t* output, std::size_t count)
    {
        volatile uint8_t* bytes = output;
        while (count--) *bytes++ = 0;
    }
}

namespace base32 {
    DecodeResult decode(const uint8_t* input, std::size_t length, uint8_t* output, std::size_t capacity, Mode mode)
    {
        std::size_t written = 0;
        const auto fail = [&](Status status) {
            erase(output, written);
            return DecodeResult{status, 0};
        };
        if (!input || !output || capacity == 0 || length == 0 ||
            length > max_encoded_bytes || (mode != Mode::strict && mode != Mode::legacy)) {
            return fail(Status::invalid_input);
        }

        uint32_t buffer = 0;
        unsigned bits = 0;
        std::size_t symbols = 0;
        std::size_t padding = 0;
        bool too_small = false;

        for (std::size_t i = 0; i < length; ++i) {
            uint8_t ch = input[i];
            if (mode == Mode::legacy) {
                if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n' || ch == '-') continue;
                if (ch >= 'a' && ch <= 'z') ch -= 'a' - 'A';
                if (ch == '0') ch = 'O';
                else if (ch == '1') ch = 'L';
                else if (ch == '8') ch = 'B';
            }
            if (mode == Mode::strict && ch == '=') {
                ++padding;
                continue;
            }
            if (padding != 0) return fail(Status::invalid_input);
            unsigned value;
            if (ch >= 'A' && ch <= 'Z') value = ch - 'A';
            else if (ch >= '2' && ch <= '7') value = ch - '2' + 26;
            else return fail(Status::invalid_input);
            ++symbols;
            buffer = (buffer << 5) | value;
            bits += 5;
            if (bits >= 8) {
                bits -= 8;
                if (written < capacity && written < max_decoded_bytes) {
                    output[written++] = static_cast<uint8_t>(buffer >> bits);
                } else {
                    too_small = true;
                }
            }
            buffer &= (uint32_t{1} << bits) - 1;
        }
        const auto remainder = symbols % 8;
        if (symbols == 0 || remainder == 1 || remainder == 3 || remainder == 6 || buffer != 0) {
            return fail(Status::invalid_input);
        }
        if (padding != 0) {
            const std::size_t needed = remainder == 2 ? 6 : remainder == 4 ? 4 :
                                    remainder == 5 ? 3 : remainder == 7 ? 1 : 0;
            if (needed == 0 || padding != needed || length % 8 != 0) {
                return fail(Status::invalid_input);
            }
        }
        if (too_small) return fail(Status::output_too_small);
        return {Status::ok, written};
    }
} // namespace base32

int base32_decode(const uint8_t* encoded, uint8_t* result, int bufSize)
{
    if (!encoded || !result || bufSize <= 0) return -1;
    std::size_t length = 0;

    while (length <= base32::max_encoded_bytes && encoded[length] != 0) {
        ++length;
    }

    const auto decoded = base32::decode(encoded, length, result, static_cast<std::size_t>(bufSize), base32::Mode::legacy);
    if (decoded.status != base32::Status::ok) return -1;
    if (decoded.length < static_cast<std::size_t>(bufSize)) result[decoded.length] = 0;
    return static_cast<int>(decoded.length);
}

int base32_encode(const uint8_t* data, int length, uint8_t* result, int bufSize)
{
    if (length < 0 || length > static_cast<int>(base32::max_decoded_bytes) ||
        bufSize < 0 || (!data && length != 0) || (!result && bufSize != 0)) return -1;

    const int required = (length * 8 + 4) / 5;

    if (bufSize < required) return -1;

    uint32_t buffer = 0;
    unsigned bits = 0;
    int count = 0;
    constexpr char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    for (int i = 0; i < length; ++i) {
        buffer = (buffer << 8) | data[i];
        bits += 8;
        while (bits >= 5) {
            bits -= 5;
            result[count++] = alphabet[(buffer >> bits) & 31];
        }
        buffer &= (uint32_t{1} << bits) - 1;
    }

    if (bits != 0) result[count++] = alphabet[(buffer << (5 - bits)) & 31];
    if (count < bufSize) result[count] = 0;
    return count;
}
