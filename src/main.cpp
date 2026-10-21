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

#include "../lib/sdk/plugincommon.h"
#include "pawn_natives.h"
#include "component.h"

using logprintf_t = void (*)(char* format, ...);
logprintf_t logprintf = nullptr;
extern void* pAMXFunctions;

namespace {
    bool legacy_available() noexcept
    {
        return pAMXFunctions && static_cast<void**>(pAMXFunctions)[PLUGIN_AMX_EXPORT_GetAddr];
    }

    int legacy_address(AMX* amx, cell address, cell** physical)
    {
        return amx_GetAddr(amx, address, physical);
    }

    void legacy_log(ComponentRuntime::LogLevel, const char* line)
    {
        if (logprintf) logprintf("%s", line);
    }
}

PLUGIN_EXPORT unsigned int PLUGIN_CALL Supports()
{
    return SUPPORTS_VERSION | SUPPORTS_AMX_NATIVES;
}

PLUGIN_EXPORT bool PLUGIN_CALL Load(void** data)
{
    if (ComponentRuntime::kind() != ComponentRuntime::Kind::none || !data || !data[PLUGIN_DATA_LOGPRINTF] || !data[PLUGIN_DATA_AMX_EXPORTS]) 
    {
        return false;
    }

    auto** functions = static_cast<void**>(data[PLUGIN_DATA_AMX_EXPORTS]);
    if (!functions[PLUGIN_AMX_EXPORT_GetAddr] || !functions[PLUGIN_AMX_EXPORT_Register]) return false;
    if (!ComponentRuntime::begin(ComponentRuntime::Kind::legacy, { legacy_available, legacy_address, legacy_log })) 
    {
        return false;
    }

    pAMXFunctions = functions;
    logprintf = reinterpret_cast<logprintf_t>(data[PLUGIN_DATA_LOGPRINTF]);
    ComponentRuntime::announce();
    return true;
}

PLUGIN_EXPORT void PLUGIN_CALL Unload()
{
    if (ComponentRuntime::kind() != ComponentRuntime::Kind::legacy) return;
    ComponentRuntime::end(ComponentRuntime::Kind::legacy);
    pAMXFunctions = nullptr;
    logprintf = nullptr;
}

PLUGIN_EXPORT int PLUGIN_CALL AmxLoad(AMX* amx)
{
    if (ComponentRuntime::kind() != ComponentRuntime::Kind::legacy || !amx || !pAMXFunctions || !static_cast<void**>(pAMXFunctions)[PLUGIN_AMX_EXPORT_Register])
    {
        return AMX_ERR_INIT;
    }
    
    return amx_Register(amx, PluginNatives, -1);
}

PLUGIN_EXPORT int PLUGIN_CALL AmxUnload(AMX*)
{
    return AMX_ERR_NONE;
}
