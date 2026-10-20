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

#include "../lib/sdk/amx/amx.h"
#include "../lib/sdk/plugincommon.h"

#include <cstdint>

#include "main.h"
#include "totp.h"
#include "base32.h"
#include "crypto.h"
#include "pawn_natives.h"

#define TIME_INTERVAL 30

typedef void(*logprintf_t)(char* format, ...);

logprintf_t logprintf;
extern void *pAMXFunctions;

int compute_code(const char* key, unsigned long counter)
{
    const auto result = totp::generate_counter(key, static_cast<std::uint32_t>(counter));
    return result.status == totp::Status::ok ? static_cast<int>(result.code) : -1;
}

namespace {
	struct SecretBuffer {
		char text[base32::max_encoded_bytes + 1]{};
		crypto::ScopedCleanse cleanup{text, sizeof(text)};
	};

	bool read_secret(AMX* amx, cell address, SecretBuffer& secret)
	{
		if (!amx || (!amx->data && !amx->base) || !pAMXFunctions ||
			!static_cast<void**>(pAMXFunctions)[PLUGIN_AMX_EXPORT_GetAddr] ||
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
			if (amx_GetAddr(amx, static_cast<cell>(offset), &physical) != AMX_ERR_NONE || !physical ||
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
} // namespace

cell AMX_NATIVE_CALL GoogleAuthenticatorCode(AMX* amx, cell* params)
{
    if (!params) return -1;
    if (params[0] != 8) {
        if (logprintf) {
            logprintf("GoogleAuthenticatorCode: Incorrect number of arguments (%d). Expecting 2 parameters.",
                      params[0] / static_cast<cell>(sizeof(cell)));
        }
        return -1;
    }
    SecretBuffer secret;
    if (!read_secret(amx, params[1], secret)) return -1;
    return compute_code(secret.text, static_cast<int>(params[2]) / TIME_INTERVAL);
}


PLUGIN_EXPORT unsigned int PLUGIN_CALL Supports()
{
	return SUPPORTS_VERSION | SUPPORTS_AMX_NATIVES;
}

PLUGIN_EXPORT bool PLUGIN_CALL Load(void **ppData)
{
	pAMXFunctions = ppData[PLUGIN_DATA_AMX_EXPORTS];
	logprintf = (logprintf_t)ppData[PLUGIN_DATA_LOGPRINTF];

	logprintf("--------------------------------------------");
	logprintf("  TOTP Plugin v%s", TOTP_PLUGIN_VER);
	logprintf("  Author: Games");
	logprintf("  Modified by: Le Tuan Kiet (aujiz11)");
	logprintf("  See more https://github.com/aujiz11/TOTP");
	logprintf("--------------------------------------------");
	return true;
}

PLUGIN_EXPORT void PLUGIN_CALL Unload()
{
	logprintf("TOTP Plugin v%s unloaded.", TOTP_PLUGIN_VER);
}

AMX_NATIVE_INFO PluginNatives[] =
{
	{ "GoogleAuthenticatorCode", GoogleAuthenticatorCode },
    { "TOTP_GenerateCode", TOTP_GenerateCode },
    { "TOTP_VerifyCode", TOTP_VerifyCode },
    { "TOTP_GenerateSecret", TOTP_GenerateSecret },
    { "TOTP_GetVersion", TOTP_GetVersion },
	{ 0, 0 }
};

PLUGIN_EXPORT int PLUGIN_CALL AmxLoad(AMX *amx)
{
	return amx_Register(amx, PluginNatives, -1);
}


PLUGIN_EXPORT int PLUGIN_CALL AmxUnload(AMX *amx)
{
	return AMX_ERR_NONE;
}
