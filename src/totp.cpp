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
#include "hmac.h"

#include <cstring>

#define VERIFICATION_CODE_MODULUS (1000*1000)
#define SHA1_DIGEST_LENGTH 20

namespace totp {
	
	Result generate_counter(const char* key, std::uint64_t tm)
	{
		if (key == nullptr) return {Status::invalid_secret, 0};

        uint8_t secret[base32::max_decoded_bytes]{};
        const int secretLen = base32_decode(reinterpret_cast<const uint8_t*>(key),
                                          secret, static_cast<int>(sizeof(secret)));
        if (secretLen < 1) return {Status::invalid_secret, 0};

		uint8_t val[8];
		for (int i = 8; i--; tm >>= 8) {
			val[i] = static_cast<uint8_t>(tm);
		}
		uint8_t hash[SHA1_DIGEST_LENGTH];
		hmac_sha1(secret, secretLen, val, 8, hash, SHA1_DIGEST_LENGTH);
		std::memset(val, 0, sizeof(val));
		int offset = hash[SHA1_DIGEST_LENGTH - 1] & 0xF;
		std::uint32_t truncatedHash = 0;

		for (int i = 0; i < 4; ++i)
		{
			truncatedHash <<= 8;
			truncatedHash |= hash[offset + i];
		}

		std::memset(hash, 0, sizeof(hash));
		truncatedHash &= 0x7FFFFFFF;
		truncatedHash %= VERIFICATION_CODE_MODULUS;

		return {Status::ok, truncatedHash};
	}

	Result generate_at(const char* key, std::uint64_t timestamp)
	{
		return generate_counter(key, timestamp / 30);
	}
} // namespace totp
