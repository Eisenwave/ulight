#ifndef ULIGHT_CHARS_HPP
#define ULIGHT_CHARS_HPP

#include <string_view>

#include "ulight/impl/charset.hpp"

namespace ulight::detail {

template <std::size_t N>
[[nodiscard]]
consteval Charset<N> to_charset(std::u8string_view chars)
{
    Charset<N> result {};
    for (const char8_t c : chars) {
        result.insert(c);
    }
    return result;
}

[[nodiscard]]
consteval Charset256 to_charset256(std::u8string_view chars)
{
    return to_charset<256>(chars);
}

template <std::size_t N>
[[nodiscard]]
consteval Charset<N> to_charset(bool predicate(char8_t))
{
    static_assert(N >= 256);
    Charset<N> result {};
    for (std::size_t i = 0; i < 256; ++i) {
        if (predicate(char8_t(i))) {
            result.insert(char8_t(i));
        }
    }
    return result;
}

[[nodiscard]]
consteval Charset256 to_charset256(bool predicate(char8_t))
{
    return to_charset<256>(predicate);
}

template <std::size_t N>
[[nodiscard]]
consteval Charset<N> to_charset(char8_t c)
{
    return Charset<N> {} | c;
}

[[nodiscard]]
consteval Charset256 to_charset256(char8_t c)
{
    return to_charset<256>(c);
}

} // namespace ulight::detail

#endif
