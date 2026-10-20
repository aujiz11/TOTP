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

#ifndef TOTP_OTP_H
#define TOTP_OTP_H

#include <cstddef>
#include <cstdint>

namespace totp {
    constexpr std::size_t code_size = 7;
    constexpr std::size_t secret_size = 33;
    constexpr std::size_t version_size = 32;
    constexpr int max_skew = 2;

    bool generate_code(const char* secret, std::size_t length, std::int64_t timestamp,
                       char (&output)[code_size]);

    int verify_code(const char* secret, std::size_t length, const char* code,
                    std::size_t code_length, std::int64_t timestamp, int skew);
    bool generate_secret(char (&output)[secret_size]);
}
#endif
