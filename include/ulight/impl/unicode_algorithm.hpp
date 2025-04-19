#ifndef ULIGHT_UNICODE_ALGORITHM_HPP
#define ULIGHT_UNICODE_ALGORITHM_HPP

#include <cstddef>
#include <string_view>
#include <type_traits>

#include "ulight/impl/unicode.hpp"

namespace ulight::utf8 {

namespace detail {

template <typename F>
    requires std::is_invocable_r_v<bool, F, char32_t>
[[nodiscard]]
std::size_t find_if(std::u8string_view str, F predicate, bool expected, std::size_t npos)
{
    std::size_t code_units = 0;
    while (!str.empty()) {
        const auto [code_point, length] = decode_and_length_or_throw(str);
        if (bool(predicate(code_point)) == expected) {
            return code_units;
        }
        code_units += std::size_t(length);
        str.remove_prefix(std::size_t(length));
    }
    return npos;
}

} // namespace detail

/// @brief Returns the position of the first code point `c` in `str` for which
/// `predicate(c)` is `true`, in code units.
/// If none could be found, returns `std::u8string_view::npos`.
/// @throws Unicode_Error If decoding failed.
template <typename F>
    requires std::is_invocable_r_v<bool, F, char32_t>
[[nodiscard]]
constexpr std::size_t find_if(std::u8string_view str, F predicate)
{
    return detail::find_if(str, predicate, true, std::u8string_view::npos);
}

/// @brief Returns the position of the first code point `c` in `str` for which
/// `predicate(c)` is `false`, in code units.
/// If none could be found, returns `std::u8string_view::npos`.
/// @throws Unicode_Error If decoding failed.
template <typename F>
    requires std::is_invocable_r_v<bool, F, char32_t>
[[nodiscard]]
constexpr std::size_t find_if_not(std::u8string_view str, F predicate)
{
    return detail::find_if(str, predicate, false, std::u8string_view::npos);
}

/// @brief Like `find_if`, but returns `str.length()` instead of `npos`.
template <typename F>
    requires std::is_invocable_r_v<bool, F, char32_t>
[[nodiscard]]
constexpr std::size_t length_if(std::u8string_view str, F predicate)
{
    return detail::find_if(str, predicate, false, str.length());
}

/// @brief Like `find_if_not`, but returns `str.length()` instead of `npos`.
template <typename F>
    requires std::is_invocable_r_v<bool, F, char32_t>
[[nodiscard]]
constexpr std::size_t length_if_not(std::u8string_view str, F predicate)
{
    return detail::find_if(str, predicate, true, str.length());
}

} // namespace ulight::utf8

#endif
