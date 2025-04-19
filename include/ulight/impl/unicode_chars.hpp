#ifndef ULIGHT_UNICODE_CHARS_HPP
#define ULIGHT_UNICODE_CHARS_HPP

#include "ulight/impl/ascii_chars.hpp"
#include "ulight/impl/chars.hpp"

namespace ulight {

/// @brief The greatest value for which `is_ascii` is `true`.
inline constexpr char32_t code_point_max_ascii = U'\u007f';
/// @brief The greatest value for which `is_code_point` is `true`.
inline constexpr char32_t code_point_max = U'\U0010FFFF';

constexpr bool is_code_point(char8_t c) = delete;

[[nodiscard]]
constexpr bool is_code_point(char32_t c)
{
    // https://infra.spec.whatwg.org/#code-point
    return c <= code_point_max;
}

constexpr bool is_leading_surrogate(char8_t c) = delete;

[[nodiscard]]
constexpr bool is_leading_surrogate(char32_t c)
{
    // https://infra.spec.whatwg.org/#leading-surrogate
    return c >= 0xD800 && c <= 0xDBFF;
}

constexpr bool is_trailing_surrogate(char8_t c) = delete;

[[nodiscard]]
constexpr bool is_trailing_surrogate(char32_t c)
{
    // https://infra.spec.whatwg.org/#trailing-surrogate
    return c >= 0xDC00 && c <= 0xDFFF;
}

constexpr bool is_surrogate(char8_t c) = delete;

[[nodiscard]]
constexpr bool is_surrogate(char32_t c)
{
    // https://infra.spec.whatwg.org/#surrogate
    return c >= 0xD800 && c <= 0xDFFF;
}

constexpr bool is_scalar_value(char8_t c) = delete;

/// @brief Returns `true` iff `c` is a scalar value,
/// i.e. a code point that is not a surrogate.
/// Only scalar values can be encoded via UTF-8.
[[nodiscard]]
constexpr bool is_scalar_value(char32_t c)
{
    // https://infra.spec.whatwg.org/#scalar-value
    return is_code_point(c) && !is_surrogate(c);
}

constexpr bool is_noncharacter(char8_t c) = delete;

/// @brief Returns `true` if `c` is a noncharacter,
/// i.e. if it falls outside the range of valid code points.
[[nodiscard]]
constexpr bool is_noncharacter(char32_t c)
{
    // https://infra.spec.whatwg.org/#noncharacter
    if ((c >= U'\uFDD0' && c <= U'\uFDEF') || (c >= U'\uFFFE' && c <= U'\uFFFF')) {
        return true;
    }
    // This includes U+11FFFF, which is not a noncharacter but simply not a valid code point.
    // We don't make that distinction here.
    const auto lower = c & 0xffff;
    return lower >= 0xfffe && lower <= 0xffff;
}

// https://unicode.org/charts/PDF/UE000.pdf
inline constexpr char32_t private_use_area_min = U'\uE000';
// https://unicode.org/charts/PDF/UE000.pdf
inline constexpr char32_t private_use_area_max = U'\uF8FF';
// https://unicode.org/charts/PDF/UF0000.pdf
inline constexpr char32_t supplementary_pua_a_min = U'\U000F0000';
// https://unicode.org/charts/PDF/UF0000.pdf
inline constexpr char32_t supplementary_pua_a_max = U'\U000FFFFF';
// https://unicode.org/charts/PDF/U100000.pdf
inline constexpr char32_t supplementary_pua_b_min = U'\U00100000';
// https://unicode.org/charts/PDF/UF0000.pdf
inline constexpr char32_t supplementary_pua_b_max = U'\U0010FFFF';

constexpr bool is_private_use_area_character(char8_t c) = delete;

/// @brief Returns `true` iff `c` is a noncharacter,
/// i.e. if it falls outside the range of valid code points.
[[nodiscard]]
constexpr bool is_private_use_area_character(char32_t c)
{
    return (c >= private_use_area_min && c <= private_use_area_max) //
        || (c >= supplementary_pua_a_min && c <= supplementary_pua_a_max) //
        || (c >= supplementary_pua_b_min && c <= supplementary_pua_b_max);
}

inline constexpr Charset256 is_ascii_xid_start_set = is_ascii_alpha_set;

/// @brief Equivalent to `is_ascii_alpha(c)`.
[[nodiscard]]
constexpr bool is_ascii_xid_start(char8_t c) noexcept
{
    return is_ascii_alpha(c);
}

/// @brief Equivalent to `is_ascii_alpha(c)`.
[[nodiscard]]
constexpr bool is_ascii_xid_start(char32_t c) noexcept
{
    return is_ascii_alpha(c);
}

bool is_xid_start(char8_t c) = delete;

/// @brief Returns `true` iff `c` has the XID_Start Unicode property.
/// This property indicates whether the character can appear at the beginning
/// of a Unicode identifier, such as a C++ *identifier*.
[[nodiscard]]
bool is_xid_start(char32_t c) noexcept;

/// @brief Returns `true` iff `c` is in the set `[a-zA-Z0-9_]`.
[[nodiscard]]
constexpr bool is_ascii_xid_continue(char8_t c) noexcept
{
    return is_ascii_alphanumeric(c) || c == u8'_';
}

/// @brief Returns `true` iff `c` is in the set `[a-zA-Z0-9_]`.
[[nodiscard]]
constexpr bool is_ascii_xid_continue(char32_t c) noexcept
{
    return is_ascii_alphanumeric(c) || c == u8'_';
}

inline constexpr Charset256 is_ascii_xid_continue_set
    = detail::to_charset256(is_ascii_xid_continue);

bool is_xid_continue(char8_t c) = delete;

/// @brief Returns `true` iff `c` has the XID_Continue Unicode property.
/// This property indicates whether the character can appear
/// in a Unicode identifier, such as a C++ *identifier*.
[[nodiscard]]
bool is_xid_continue(char32_t c) noexcept;

} // namespace ulight

#endif
