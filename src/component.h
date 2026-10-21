// Copyright (C) 2026 Le Tuan Kiet (aujiz11)
// Licensed under the Apache License, Version 2.0. See LICENSE.
#ifndef TOTP_COMPONENT_H
#define TOTP_COMPONENT_H

#include "../lib/sdk/amx/amx.h"

namespace ComponentRuntime {
    enum class Kind { none, legacy, component };
    enum class LogLevel { info, warning, error };
    struct Backend {
        bool (*available)() noexcept;
        int (*get_address)(AMX*, cell, cell**);
        void (*log)(LogLevel, const char*);
    };

    Kind kind() noexcept;
    bool begin(Kind kind, const Backend& backend) noexcept;
    void announce();
    void end(Kind kind);
    bool address_api_available() noexcept;
    int get_address(AMX* amx, cell address, cell** physical);
    void invalid_legacy_arguments(cell count);
}

#endif
