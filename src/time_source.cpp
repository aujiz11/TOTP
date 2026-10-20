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

#include "time_source.h"
#include <ctime>
#include <limits>

static_assert(sizeof(std::time_t) == sizeof(std::int64_t) && std::numeric_limits<std::time_t>::is_signed, 
    "A signed 64-bit Unix clock is required."
);

bool totp::unix_time_now(std::int64_t& seconds) noexcept
{
    seconds = 0;
    const auto now = std::time(nullptr);
    if (now < 0) return false; // Includes the time() error sentinel -1.
    seconds = static_cast<std::int64_t>(now);
    return true;
}
