// Copyright (C) 2026 Le Tuan Kiet (aujiz11)
// Licensed under the Apache License, Version 2.0. See LICENSE.

#include "component.h"
#include "pawn_natives.h"
#include "main.h"
#include "generated/totp_build_config.h"
#include <sdk.hpp>
#include <Server/Components/Pawn/pawn.hpp>
#include <unordered_map>
#include <cstdio>

namespace {
    ComponentRuntime::Kind active = ComponentRuntime::Kind::none;
    ComponentRuntime::Backend backend{};
    bool announced = false;

    const char* runtime_name() noexcept
    {
        return active == ComponentRuntime::Kind::component ? "Component" : "Plugin";
    }

    ICore* core = nullptr;
    IPawnComponent* pawn = nullptr;
    std::unordered_map<AMX*, IPawnScript*> scripts;

    bool component_available() noexcept 
    {
        return pawn != nullptr; 
    }

    int component_address(AMX* amx, cell address, cell** physical)
    {
        const auto found = scripts.find(amx);
        return found != scripts.end() ? found->second->GetAddr(address, physical) : AMX_ERR_INIT;
    }

    void component_log(ComponentRuntime::LogLevel level, const char* line)
    {
        if (!core) return;
        if (level == ComponentRuntime::LogLevel::warning) {
            core->logLn(LogLevel::Warning, "%s", line);
        }
        else if (level == ComponentRuntime::LogLevel::error) {
            core->logLn(LogLevel::Error, "%s", line);
        }
        else core->printLn("%s", line);
    }

    class TOTPComponent final : public IComponent, public PawnEventHandler {
    public:
        PROVIDE_UID(0x6d87e8d35584b8a1ULL);

        StringView componentName() const override { return StringView("TOTP"); }
        SemanticVersion componentVersion() const override
        {
            return SemanticVersion(TOTP_VERSION_MAJOR, TOTP_VERSION_MINOR, TOTP_VERSION_PATCH);
        }

        void onLoad(ICore* value) override
        {
            if (core) return;
            core = value;
            ComponentRuntime::announce();
        }

        void onInit(IComponentList* components) override
        {
            pawn = components->queryComponent<IPawnComponent>();
            if (!pawn) {
                core->logLn(LogLevel::Error, "TOTP: the open.mp Pawn component is required.");
                return;
            }
            pawn->getEventDispatcher().addEventHandler(this);
        }

        void onReady() override
        {
            if (!pawn) return;
            if (auto* script = pawn->mainScript(); script && script->IsLoaded()) onAmxLoad(*script);
            for (auto* script : pawn->sideScripts()) {
                if (script && script->IsLoaded()) onAmxLoad(*script);
            }
        }

        void onAmxLoad(IPawnScript& script) override
        {
            scripts[script.GetAMX()] = &script;
            const int error = script.Register(PluginNatives, -1);
            if (error != AMX_ERR_NONE && error != AMX_ERR_NOTFOUND && core) {
                core->logLn(LogLevel::Error, "TOTP: native registration failed (AMX error %d).", error);
            }
        }
        void onAmxUnload(IPawnScript& script) override { 
            scripts.erase(script.GetAMX()); 
        }
        void reset() override { }

        void onFree(IComponent* component) override
        {
            if (component == pawn) detachPawn();
        }

        void free() override
        {
            detachPawn();
            ComponentRuntime::end(ComponentRuntime::Kind::component);
            core = nullptr;
            delete this;
        }

    private:
        void detachPawn()
        {
            if (pawn) pawn->getEventDispatcher().removeEventHandler(this);
            pawn = nullptr;
            scripts.clear();
        }
    };
}

namespace ComponentRuntime {
    Kind kind() noexcept { return active; }

    bool begin(Kind requested, const Backend& api) noexcept
    {
        if (active != Kind::none || requested == Kind::none ||
            !api.available || !api.get_address || !api.log) return false;
        backend = api;
        active = requested;
        return true;
    }

    void announce()
    {
        if (active == Kind::none || announced) return;
        announced = true;
        char line[128];
        std::snprintf(line, sizeof(line), "  TOTP %s v%s", runtime_name(), TOTP_PLUGIN_VER);
        backend.log(LogLevel::info, "--------------------------------------------");
        backend.log(LogLevel::info, line);
        backend.log(LogLevel::info, "  Author: Games");
        backend.log(LogLevel::info, "  Modified by: Le Tuan Kiet (aujiz11)");
        backend.log(LogLevel::info, "  See more https://github.com/aujiz11/TOTP");
        backend.log(LogLevel::info, "--------------------------------------------");
    }

    void end(Kind owner)
    {
        if (active == Kind::none || active != owner) return;
        if (announced) {
            char line[128];
            std::snprintf(line, sizeof(line), "TOTP %s v%s unloaded.", runtime_name(), TOTP_PLUGIN_VER);
            backend.log(LogLevel::info, line);
        }
        backend = {};
        active = Kind::none;
        announced = false;
    }

    bool address_api_available() noexcept
    {
        return active != Kind::none && backend.available();
    }

    int get_address(AMX* amx, cell address, cell** physical)
    {
        return address_api_available() ? backend.get_address(amx, address, physical) : AMX_ERR_INIT;
    }

    void invalid_legacy_arguments(cell count)
    {
        if (active == Kind::none) return;
        char line[160];
        std::snprintf(line, sizeof(line),
            "GoogleAuthenticatorCode: Incorrect number of arguments (%d). Expecting 2 parameters.", count);
        backend.log(LogLevel::warning, line);
    }
}

COMPONENT_ENTRY_POINT()
{
    if (ComponentRuntime::kind() != ComponentRuntime::Kind::none) {
        return nullptr;
    }

    auto* component = new TOTPComponent();
    if (!ComponentRuntime::begin(ComponentRuntime::Kind::component, { component_available, component_address, component_log })) {
        delete component;
        return nullptr;
    }
    return component;
}
