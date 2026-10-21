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

#include "pawn_natives.h"
#include "component.h"
#include "base32.h"
#include "crypto.h"
#include "otp.h"
#include "totp.h"
#include "time_source.h"
#include "main.h"
#include <array>
#include <cstdint>

const AMX_NATIVE_INFO PluginNatives[] = {
    { "GoogleAuthenticatorCode", GoogleAuthenticatorCode },
    { "TOTP_GenerateCode", TOTP_GenerateCode },
    { "TOTP_VerifyCode", TOTP_VerifyCode },
    { "TOTP_GenerateSecret", TOTP_GenerateSecret },
    { "TOTP_GetVersion", TOTP_GetVersion },
    { nullptr, nullptr }
};

namespace {
	struct LegacySecretBuffer {
		char text[base32::max_encoded_bytes + 1]{};
		crypto::ScopedCleanse cleanup{text, sizeof(text)};
	};

	bool read_legacy_secret(AMX* amx, cell address, LegacySecretBuffer& secret)
	{
		if (!amx || (!amx->data && !amx->base) || !ComponentRuntime::address_api_available() ||
			address < 0 || address % sizeof(cell) != 0 ||
			amx->hea < 0 || amx->stk < amx->hea || amx->stp < amx->stk ||
			amx->stp <= 0 || amx->hea % sizeof(cell) != 0 ||
			amx->stk % sizeof(cell) != 0 || amx->stp % sizeof(cell) != 0) return false;

		// Read only within the starting live heap/data or stack region, never the gap.
		const cell end = address < amx->hea ? amx->hea : address >= amx->stk ? amx->stp : 0;
		if (end <= address) return false;

		bool packed = false;
		std::size_t length = 0;

		for (std::uint64_t offset = static_cast<ucell>(address); offset + sizeof(cell) <= static_cast<ucell>(end); offset += sizeof(cell))
		{
			cell* physical = nullptr;
			if (ComponentRuntime::get_address(amx, static_cast<cell>(offset), &physical) != AMX_ERR_NONE || !physical ||
				reinterpret_cast<std::uintptr_t>(physical) % alignof(cell) != 0) return false;

			if (amx->data && reinterpret_cast<std::uintptr_t>(physical) !=
				reinterpret_cast<std::uintptr_t>(amx->data) + offset) return false;

			const ucell value = static_cast<ucell>(*physical);
			if (offset == static_cast<ucell>(address)) {
				packed = value > static_cast<ucell>(UNPACKEDMAX);
			}

			const unsigned characters = packed ? sizeof(cell) : 1;
			for (unsigned i = 0; i < characters; ++i) {
				const ucell character = packed ? (value >> (8 * (sizeof(cell) - 1 - i))) & 0xff : value;
				if (character == 0) return true;
				if (character > 0xff || length == base32::max_encoded_bytes) return false;

				secret.text[length++] = static_cast<char>(character);
			}
		}
		return false;
	}

    bool region(AMX* amx, cell address, std::uint64_t cells)
    {
        if (!amx || (!amx->data && !amx->base) || !ComponentRuntime::address_api_available() ||
            address < 0 || address % sizeof(cell) != 0 || cells == 0 ||
            amx->hea < 0 || amx->stk < amx->hea || amx->stp < amx->stk || amx->stp <= 0 ||
            amx->hea % sizeof(cell) != 0 || amx->stk % sizeof(cell) != 0 || amx->stp % sizeof(cell) != 0) return false;
        const cell end = address < amx->hea ? amx->hea : address >= amx->stk ? amx->stp : 0;
        return end > address && cells <= static_cast<std::uint64_t>(end - address) / sizeof(cell);
    }

    cell* resolve(AMX* amx, cell address)
    {
        cell* pointer = nullptr;
        if (!region(amx, address, 1) || ComponentRuntime::get_address(amx, address, &pointer) != AMX_ERR_NONE ||
            !pointer || reinterpret_cast<std::uintptr_t>(pointer) % alignof(cell) != 0) return nullptr;
        if (amx->data && reinterpret_cast<std::uintptr_t>(pointer) !=
            reinterpret_cast<std::uintptr_t>(amx->data) + static_cast<ucell>(address)) return nullptr;
        return pointer;
    }

    bool overlap(std::uint64_t a, std::uint64_t a_length, std::uint64_t b, std::uint64_t b_length)
    {
        // Subtraction avoids address-end overflow.
        return a <= b ? b - a < a_length : a - b < b_length;
    }

    template<std::size_t N> struct Text {
        char data[N]{};
        crypto::ScopedCleanse cleanup{data, sizeof(data)};
        std::size_t length = 0;
        cell address = 0;
        std::size_t span = 0;
        cell* first = nullptr;

        bool read(AMX* amx, cell start)
        {
            address = start;
            bool packed = false;
            for (std::size_t index = 0; index < N; ++index) 
            {
                const auto offset = static_cast<std::int64_t>(start) + static_cast<std::int64_t>(index * sizeof(cell));
                if (offset > INT32_MAX || offset < 0) return false;
                
                cell* pointer = resolve(amx, static_cast<cell>(offset));
                if (!pointer) return false;
                
                if (index == 0) { 
                    first = pointer;
                    packed = static_cast<ucell>(*pointer) > UNPACKEDMAX;
                }

                if (reinterpret_cast<std::uintptr_t>(pointer) !=  reinterpret_cast<std::uintptr_t>(first) + index * sizeof(cell))
                {
                    return false;
                }

                span += sizeof(cell);
                const auto value = static_cast<ucell>(*pointer);
                const unsigned count = packed ? sizeof(cell) : 1;
                
                for (unsigned i = 0; i < count; ++i) 
                {
                    const auto ch = packed ? (value >> (8 * (sizeof(cell) - 1 - i))) & 0xff : value;
                    if (ch == 0) return true;
                    if (length == N - 1 || ch > 0x7f) return false;
                    data[length++] = static_cast<char>(ch);
                }
            }
            return false;
        }
    };

    struct Output {
        std::array<cell*, totp::secret_size> cells{};
        bool clearable = false;

        bool prepare(AMX* amx, cell address, cell capacity, std::size_t required, cell input_address = 0, std::size_t input_span = 0, cell* input_first = nullptr)
        {
            if (capacity <= 0 || !region(amx, address, static_cast<ucell>(capacity))) return false;
            
            const auto bytes = static_cast<std::uint64_t>(capacity) * sizeof(cell);
            if (input_span) 
            {
                if (overlap(static_cast<ucell>(address), bytes, static_cast<ucell>(input_address), input_span))
                {
                    return false;
                }
            }
            
            const auto count = capacity < static_cast<cell>(required) ? std::size_t{1} : required;
            for (std::size_t i = 0; i < count; ++i) 
            {
                cells[i] = resolve(amx, static_cast<cell>(address + i * sizeof(cell)));
                if (!cells[i]) return false;
                
                if (i != 0 && reinterpret_cast<std::uintptr_t>(cells[i]) != reinterpret_cast<std::uintptr_t>(cells[0]) + i * sizeof(cell)) 
                {
                    return false;
                }
            }

            if (input_span) 
            {
                if (overlap(reinterpret_cast<std::uintptr_t>(cells[0]), bytes, reinterpret_cast<std::uintptr_t>(input_first), input_span))
                {
                    return false;
                }
            } 
            
            clearable = true;
            return capacity >= static_cast<cell>(required);
        }

        cell fail() 
        {
            if (clearable) *cells[0] = 0;
            return -1; 
        }
        cell publish(const char* text, std::size_t length)
        {
            // All destination cells were resolved before the first write.
            for (std::size_t i = 0; i <= length; ++i) 
            {
                *cells[i] = static_cast<unsigned char>(text[i]);
            }
            return 1;
        }
    };
}

int compute_code(const char* key, unsigned long counter)
{
    const auto result = totp::generate_counter(key, static_cast<std::uint32_t>(counter));
    return result.status == totp::Status::ok ? static_cast<int>(result.code) : -1;
}

cell AMX_NATIVE_CALL GoogleAuthenticatorCode(AMX* amx, cell* params)
{
    if (!params) return -1;
    if (params[0] != 8) {
        ComponentRuntime::invalid_legacy_arguments(params[0] / static_cast<cell>(sizeof(cell)));
        return -1;
    }
    LegacySecretBuffer secret;
    if (!read_legacy_secret(amx, params[1], secret)) return -1;
    return compute_code(secret.text, static_cast<int>(params[2]) / 30);
}

cell AMX_NATIVE_CALL TOTP_GenerateCode(AMX* amx, cell* params)
{
    if (!params || params[0] != 3 * sizeof(cell)) return -1;
    Text<base32::max_encoded_bytes + 1> secret;
    if (!secret.read(amx, params[1])) return -1; // Unknown input span: do not clear output.
    
    Output output;
    if (!output.prepare(amx, params[2], params[3], totp::code_size, secret.address, secret.span, secret.first)) 
    {
        return output.fail();
    }

    std::int64_t now = 0;
    char code[totp::code_size]{};
    crypto::ScopedCleanse cleanup(code, sizeof(code));
    if (!totp::unix_time_now(now) || !totp::generate_code(secret.data, secret.length, now, code)) 
    {
        return output.fail();
    }

    return output.publish(code, totp::code_size - 1);
}

cell AMX_NATIVE_CALL TOTP_VerifyCode(AMX* amx, cell* params)
{
    if (!params || params[0] != 3 * sizeof(cell) || params[3] < 0 || params[3] > totp::max_skew) return -1;
    
    Text<base32::max_encoded_bytes + 1> secret;
    Text<totp::code_size> code;
    
    if (!secret.read(amx, params[1]) || !code.read(amx, params[2])) return -1;
    
    std::int64_t now = 0;
    if (!totp::unix_time_now(now)) return -1;
    return totp::verify_code(secret.data, secret.length, code.data, code.length, now, params[3]);
}

cell AMX_NATIVE_CALL TOTP_GenerateSecret(AMX* amx, cell* params)
{
    if (!params || params[0] != 2 * sizeof(cell)) return -1;
   
    Output output;
    if (!output.prepare(amx, params[1], params[2], totp::secret_size)) 
    {
        return output.fail();
    }

    char secret[totp::secret_size]{};
    crypto::ScopedCleanse cleanup(secret, sizeof(secret));
    if (!totp::generate_secret(secret)) 
    {
        return output.fail();
    }
    return output.publish(secret, totp::secret_size - 1);
}

cell AMX_NATIVE_CALL TOTP_GetVersion(AMX* amx, cell* params)
{
    if (!params || params[0] != 2 * sizeof(cell)) return -1;

    Output output;
    if (!output.prepare(amx, params[1], params[2], totp::version_size)) 
    {
        return output.fail();
    }

    constexpr char version[] = TOTP_PLUGIN_VER;
    if (sizeof(version) < 2 || sizeof(version) > totp::version_size) 
    {
        return output.fail();
    }
    
    for (std::size_t i = 0; i + 1 < sizeof(version); ++i) if (version[i] < 0x21 || version[i] > 0x7e) 
    {
        return output.fail();
    }
    
    return output.publish(version, sizeof(version) - 1);
}
