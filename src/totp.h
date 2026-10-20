// Copyright (C) 2016 Games
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

#ifndef TOTP_CORE_H
#define TOTP_CORE_H

#include <cstdint>
#include <cstddef>

namespace totp {
    enum class Status : std::uint8_t {
        ok,
        invalid_secret,
        crypto_error,
        invalid_time
    };

    struct Result {
        Status status;
        std::uint32_t code; // Six-digit numeric SHA1 result
    };

    Result generate_counter(const char* key, std::uint64_t counter);
    Result generate_at(const char* key, std::uint64_t timestamp);
    Result generate_at_unix(const char* key, std::int64_t timestamp);
    Result generate_counter_strict(const char* key, std::size_t length, std::uint64_t counter);
} // namespace totp

#endif
