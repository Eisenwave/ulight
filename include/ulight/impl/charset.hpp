#ifndef ULIGHT_CHARSET_HPP
#define ULIGHT_CHARSET_HPP

#include <cstdint>

#include "ulight/impl/assert.hpp"

namespace ulight {

template <std::size_t N>
struct Charset {
    static constexpr std::size_t width = N;
    static constexpr std::size_t limb_width = 64;
    static constexpr std::size_t limb_count = (N / 64) + (N % 64 != 0);
    using limb_type = std::uint64_t;

private:
    limb_type limbs[limb_count] {};

public:
    [[nodiscard]]
    Charset()
        = default;

    [[nodiscard]]
    constexpr bool contains(char8_t c) const
    {
        return get(std::size_t(c));
    }

    constexpr void clear() noexcept
    {
        *this = {};
    }

    constexpr void remove(char8_t c)
    {
        clear(std::size_t(c));
    }

    constexpr void insert(char8_t c)
    {
        set(std::size_t(c));
    }

    [[nodiscard]]
    friend constexpr bool operator==(const Charset& x, const Charset& y)
        = default;

    [[nodiscard]]
    constexpr Charset operator~() const noexcept
    {
        Charset result;
        for (std::size_t i = 0; i < limb_count; ++i) {
            result.limbs[i] = ~limbs[i];
        }
        return result;
    }

    constexpr Charset& operator|=(const Charset& other) noexcept
    {
        for (std::size_t i = 0; i < limb_count; ++i) {
            limbs[i] |= other.limbs[i];
        }
        return *this;
    }

    constexpr Charset& operator|=(char8_t c)
    {
        set(std::size_t(c));
        return *this;
    }

    [[nodiscard]]
    friend constexpr Charset operator|(const Charset& x, const Charset& y) noexcept
    {
        Charset result = x;
        result |= y;
        return result;
    }

    [[nodiscard]]
    friend constexpr Charset operator|(const Charset& x, char8_t c)
    {
        Charset result = x;
        result |= c;
        return result;
    }

    constexpr Charset& operator&=(const Charset& other)
    {
        for (std::size_t i = 0; i < limb_count; ++i) {
            limbs[i] &= other.limbs[i];
        }
        return *this;
    }

    [[nodiscard]]
    friend constexpr Charset operator&(const Charset& x, const Charset& y) noexcept
    {
        Charset result = x;
        result &= y;
        return result;
    }

    constexpr Charset& operator-=(const Charset& other) noexcept
    {
        for (std::size_t i = 0; i < limb_count; ++i) {
            limbs[i] &= ~other.limbs[i];
        }
        return *this;
    }

    constexpr Charset& operator-=(char8_t c)
    {
        clear(std::size_t(c));
        return *this;
    }

    [[nodiscard]]
    friend constexpr Charset operator-(const Charset& x, const Charset& y) noexcept
    {
        Charset result = x;
        result -= y;
        return result;
    }

    [[nodiscard]]
    friend constexpr Charset operator-(const Charset& x, char8_t c) noexcept
    {
        Charset result = x;
        result -= c;
        return result;
    }

private:
    [[nodiscard]]
    constexpr bool get(std::size_t i) const
    {
        ULIGHT_DEBUG_ASSERT(i < width);
        return (limbs[i / limb_width] >> (i % limb_width)) & 1;
    }

    constexpr void clear(std::size_t i)
    {
        ULIGHT_DEBUG_ASSERT(i < width);
        limbs[i / limb_width] &= ~(std::uint64_t { 1 } << (i % limb_width));
    }

    constexpr void set(std::size_t i)
    {
        ULIGHT_DEBUG_ASSERT(i < width);
        limbs[i / limb_width] |= std::uint64_t { 1 } << (i % limb_width);
    }

    constexpr void set(std::size_t i, bool value)
    {
        ULIGHT_DEBUG_ASSERT(i < width);
        clear(i);
        limbs[i / limb_width] |= std::uint64_t { value } << (i % limb_width);
    }
};

using Charset128 = Charset<128>;
using Charset256 = Charset<256>;

} // namespace ulight

#endif
