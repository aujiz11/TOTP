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

#include "totp.h"
#include "base32.h"
#include "crypto.h"

#define VERIFICATION_CODE_MODULUS (1000*1000)
#define SHA1_DIGEST_LENGTH 20

namespace totp {
	
    namespace {
        Result generate(const char* key, std::size_t length, base32::Mode mode, std::uint64_t tm)
        {
            if (key == nullptr) return {Status::invalid_secret, 0};

            crypto::Sha1Digest hash{};
            crypto::ScopedCleanse erase_hash(hash.data(), hash.size());
            {
                uint8_t secret[base32::max_decoded_bytes]{};
                crypto::ScopedCleanse erase_secret(secret, sizeof(secret));
                const auto decoded = base32::decode(reinterpret_cast<const uint8_t*>(key), length, secret, sizeof(secret), mode);
                if (decoded.status != base32::Status::ok || decoded.length == 0) 
                {
                    return {Status::invalid_secret, 0};
                }

                uint8_t counter[8]{};
                crypto::ScopedCleanse erase_counter(counter, sizeof(counter));
                for (unsigned i = 0; i < sizeof(counter); ++i) 
                {
                    counter[sizeof(counter) - 1 - i] = static_cast<uint8_t>(tm >> (i * 8));
                }

                if (!crypto::hmac_sha1(secret, decoded.length, counter, sizeof(counter), hash)) 
                {
                    return {Status::crypto_error, 0};
                }
            }

            int offset = hash[SHA1_DIGEST_LENGTH - 1] & 0xF;
            std::uint32_t truncatedHash = 0;

            for (int i = 0; i < 4; ++i)
            {
                truncatedHash <<= 8;
                truncatedHash |= hash[offset + i];
            }

            truncatedHash &= 0x7FFFFFFF;
            truncatedHash %= VERIFICATION_CODE_MODULUS;

            return {Status::ok, truncatedHash};
        }
    } // namespace

    Result generate_counter(const char* key, std::uint64_t counter)
    {
        if (!key) return {Status::invalid_secret, 0};

        std::size_t length = 0;
        while (length <= base32::max_encoded_bytes && key[length] != 0) 
        {
            ++length;
        }

        return generate(key, length, base32::Mode::legacy, counter);
    }

    Result generate_counter_strict(const char* key, std::size_t length, std::uint64_t counter)
    {
        return generate(key, length, base32::Mode::strict, counter);
    }

	Result generate_at(const char* key, std::uint64_t timestamp)
	{
		return generate_counter(key, timestamp / 30);
	}

    Result generate_at_unix(const char* key, std::int64_t timestamp)
    {
        if (timestamp < 0) return {Status::invalid_time, 0};
        return generate_at(key, static_cast<std::uint64_t>(timestamp));
    }
} // namespace totp
