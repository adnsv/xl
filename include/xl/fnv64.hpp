#pragma once

#include <cstdint>

namespace xl {

static constexpr auto fnv64_offset = uint64_t{14695981039346656037u};
static constexpr auto fnv64_prime = uint64_t{1099511628211u};

auto fnv64(void const* data, unsigned long long n, uint64_t offset = fnv64_offset) -> uint64_t
{
    auto p = reinterpret_cast<uint8_t const*>(data);
    auto const end = p + n;
    auto hash = offset;
    for (; p != end; ++p) {
        hash *= fnv64_prime;
        hash ^= uint64_t(*p);
    }
    return hash;
}

} // namespace xl