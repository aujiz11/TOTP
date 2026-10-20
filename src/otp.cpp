// Copyright (C) 2026 Le Tuan Kiet (aujiz11)
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

#include "otp.h"
#include "totp.h"
#include "crypto.h"
#include "base32.h"
#include <limits>

namespace {
    void format_code(std::uint32_t value, char (&output)[totp::code_size])
    {
        output[6] = 0;
        for (int i = 5; i >= 0; --i) {
            output[i] = static_cast<char>('0' + value % 10);
            value /= 10;
        }
    }
}

namespace totp {
    bool generate_code(const char* secret, std::size_t length, std::int64_t timestamp, char (&output)[code_size])
    {
        crypto::secure_erase(output, sizeof(output));
        if (timestamp < 0) return false;
        const auto result = generate_counter_strict(secret, length, static_cast<std::uint64_t>(timestamp) / 30);
        if (result.status != Status::ok) return false;
        format_code(result.code, output);
        return true;
    }

    int verify_code(const char* secret, std::size_t length, const char* code, std::size_t code_length, std::int64_t timestamp, int skew)
    {
        if (!code || code_length != 6 || timestamp < 0 || skew < 0 || skew > max_skew) return -1;
        for (std::size_t i = 0; i < code_length; ++i) if (code[i] < '0' || code[i] > '9') return -1;
        
        const auto current = static_cast<std::uint64_t>(timestamp) / 30;
        char expected[code_size]{};
        crypto::ScopedCleanse erase_expected(expected, sizeof(expected));
        unsigned matched = 0;
        
        for (int offset = -skew; offset <= skew; ++offset) {
            const auto distance = static_cast<std::uint64_t>(offset < 0 ? -offset : offset);
            if ((offset < 0 && current < distance) || (offset > 0 && current > std::numeric_limits<std::uint64_t>::max() - distance)) 
            {
                continue;
            }

            const auto counter = offset < 0 ? current - distance : current + distance;
            const auto result = generate_counter_strict(secret, length, counter);
            if (result.status != Status::ok) return -1;
            format_code(result.code, expected);
            matched |= static_cast<unsigned>(crypto::constant_time_equal(expected, code, 6));
        }
        return matched != 0 ? 1 : 0;
    }

    bool generate_secret(char (&output)[secret_size])
    {
        crypto::secure_erase(output, sizeof(output));
        crypto::ScopedCleanse failed_output(output, sizeof(output));
        crypto::SecretBytes random{};
        crypto::ScopedCleanse erase_random(random.data(), random.size());
        
        if (!crypto::random_secret(random)) return false;
        if (base32_encode(random.data(), static_cast<int>(random.size()),
                        reinterpret_cast<std::uint8_t*>(output), static_cast<int>(sizeof(output))) != 32) 
        {
            return false;
        }
        
        failed_output.release();
        return true;
    }
}
