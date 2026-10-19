// Copyright (C) 2026 Le Tuan Kiet (aujiz11)
// OpenSSL adapter for TOTP, originally authored by Games.
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

#ifndef TOTP_CRYPTO_H
#define TOTP_CRYPTO_H

#include <array>
#include <cstddef>
#include <cstdint>

namespace crypto {
    void secure_erase(void* memory, std::size_t length) noexcept;

    class ScopedCleanse final {
    public:
        ScopedCleanse(void* memory, std::size_t length) noexcept : memory_(memory), length_(length) {}
        ~ScopedCleanse() noexcept;
        ScopedCleanse(const ScopedCleanse&) = delete;
        ScopedCleanse& operator=(const ScopedCleanse&) = delete;
        ScopedCleanse(ScopedCleanse&&) = delete;
        ScopedCleanse& operator=(ScopedCleanse&&) = delete;
        void release() noexcept { memory_ = nullptr; length_ = 0; }
    private:
        void* memory_;
        std::size_t length_;
    };

    using Sha1Digest = std::array<std::uint8_t, 20>;
    using SecretBytes = std::array<std::uint8_t, 20>;

    bool random_secret(SecretBytes& output);
    bool constant_time_equal(const void* left, const void* right, std::size_t length) noexcept;
    
    bool hmac_sha1(const std::uint8_t* key, std::size_t key_length,
                const std::uint8_t* data, std::size_t data_length,
                Sha1Digest& output);
}

#endif
