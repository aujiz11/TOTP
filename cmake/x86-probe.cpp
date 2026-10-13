#include <cstdint>
#include <string>

#if !defined(_M_IX86) && !defined(__i386__)
#error TOTP requires an x86 compiler target
#endif

static_assert(sizeof(void*) == 4, "TOTP requires 32-bit pointers");
static_assert(sizeof(unsigned long) == 4, "TOTP requires the legacy x86 counter width");
static_assert(sizeof(std::uint32_t) == 4, "TOTP requires 32-bit integer support");

int main()
{
    const std::string architecture("x86");
    return architecture.size() == 3 ? 0 : 1;
}
