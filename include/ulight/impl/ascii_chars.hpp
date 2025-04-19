#ifndef ULIGHT_ASCII_CHARS_HPP
#define ULIGHT_ASCII_CHARS_HPP

#include "ulight/impl/chars.hpp"

namespace ulight {

// PURE ASCII ======================================================================================

[[nodiscard]]
constexpr bool is_ascii(char8_t c)
{
    return c <= u8'\u007f';
}

[[nodiscard]]
constexpr bool is_ascii(char32_t c)
{
    return c <= U'\u007f';
}

inline constexpr Charset256 is_ascii_set = detail::to_charset256(is_ascii);

/// @brief Returns `true` if the `c` is a decimal digit (`0` through `9`).
[[nodiscard]]
constexpr bool is_ascii_digit(char8_t c)
{
    return c >= u8'0' && c <= u8'9';
}

/// @brief Returns `true` if the `c` is a decimal digit (`0` through `9`).
[[nodiscard]]
constexpr bool is_ascii_digit(char32_t c)
{
    return c >= U'0' && c <= U'9';
}

inline constexpr Charset256 is_ascii_digit_set = detail::to_charset256(is_ascii_digit);

/// @brief Returns `true` if `c` is a digit in the usual representation of digits beyond base 10.
/// That is, after `9`, the next digit is `a`, then `b`, etc.
/// For example, `is_ascii_digit_base(c, 16)` is equivalent to `is_ascii_hex_digit(c)`.
/// i.e. whether it is a "C-style" hexadecimal digit.
/// @param base The base, in range [1, 62].
[[nodiscard]]
constexpr bool is_ascii_digit_base(char8_t c, int base)
{
    ULIGHT_DEBUG_ASSERT(base > 0 && base <= 62);

    if (base < 10) {
        return c >= u8'0' && int(c) < int(u8'0') + base;
    }
    return is_ascii_digit(c) || //
        (c >= u8'a' && int(c) < int(u8'a') + base - 10) || //
        (c >= u8'A' && int(c) < int(u8'A') + base - 10);
}

/// @brief See the `char8_t` overload.
[[nodiscard]]
constexpr bool is_ascii_digit_base(char32_t c, int base)
{
    return is_ascii(c) && is_ascii_digit_base(char8_t(c), base);
}

/// @brief Returns `true` if `c` is `'0'` or `'1'`.
[[nodiscard]]
constexpr bool is_ascii_binary_digit(char8_t c)
{
    return c == u8'0' || c == u8'1';
}

/// @brief Returns `true` if `c` is `'0'` or `'1'`.
[[nodiscard]]
constexpr bool is_ascii_binary_digit(char32_t c)
{
    return c == U'0' || c == U'1';
}

inline constexpr Charset256 is_ascii_binary_digit_set
    = detail::to_charset256(is_ascii_binary_digit);

/// @brief Returns `true` if `c` is in `[0-7]`.
[[nodiscard]]
constexpr bool is_ascii_octal_digit(char8_t c)
{
    return c >= u8'0' && c <= u8'7';
}

/// @brief Returns `true` if `c` is in `[0-7]`.
[[nodiscard]]
constexpr bool is_ascii_octal_digit(char32_t c)
{
    return c >= U'0' && c <= U'7';
}

inline constexpr Charset256 is_ascii_octal_digit_set = detail::to_charset256(is_ascii_octal_digit);

/// @brief Returns `true` if `c` is in `[0-9A-Fa-f]`.
[[nodiscard]]
constexpr bool is_ascii_hex_digit(char8_t c)
{
    // TODO: remove the C++/Lua-specific versions in favor of this.
    return is_ascii_digit_base(c, 16);
}

/// @brief Returns `true` if `c` is in `[0-9A-Fa-f]`.
[[nodiscard]]
constexpr bool is_ascii_hex_digit(char32_t c)
{
    return is_ascii_digit_base(c, 16);
}

inline constexpr Charset256 is_ascii_hex_digit_set = detail::to_charset256(is_ascii_hex_digit);

[[nodiscard]]
constexpr bool is_ascii_upper_alpha(char8_t c)
{
    // https://infra.spec.whatwg.org/#ascii-upper-alpha
    return c >= u8'A' && c <= u8'Z';
}

[[nodiscard]]
constexpr bool is_ascii_upper_alpha(char32_t c)
{
    // https://infra.spec.whatwg.org/#ascii-upper-alpha
    return c >= U'A' && c <= U'Z';
}

inline constexpr Charset256 is_ascii_upper_alpha_set = detail::to_charset256(is_ascii_upper_alpha);

[[nodiscard]]
constexpr bool is_ascii_lower_alpha(char8_t c)
{
    // https://infra.spec.whatwg.org/#ascii-lower-alpha
    return c >= u8'a' && c <= u8'z';
}

[[nodiscard]]
constexpr bool is_ascii_lower_alpha(char32_t c)
{
    // https://infra.spec.whatwg.org/#ascii-lower-alpha
    return c >= U'a' && c <= U'z';
}

inline constexpr Charset256 is_ascii_lower_alpha_set = detail::to_charset256(is_ascii_lower_alpha);

/// @brief If `is_ascii_lower_alpha(c)` is `true`,
/// returns the corresponding upper case alphabetic character, otherwise `c`.
[[nodiscard]]
constexpr char8_t to_ascii_upper(char8_t c)
{
    return is_ascii_lower_alpha(c) ? c & 0xdf : c;
}

/// @brief If `is_ascii_upper_alpha(c)` is `true`,
/// returns the corresponding lower case alphabetic character, otherwise `c`.
[[nodiscard]]
constexpr char8_t to_ascii_lower(char8_t c)
{
    return is_ascii_upper_alpha(c) ? c | 0x20 : c;
}

/// @brief Returns `true` if `c` is a latin character (`[a-zA-Z]`).
[[nodiscard]]
constexpr bool is_ascii_alpha(char8_t c)
{
    // https://infra.spec.whatwg.org/#ascii-alpha
    return is_ascii_lower_alpha(c) || is_ascii_upper_alpha(c);
}

/// @brief Returns `true` if `c` is a latin character (`[a-zA-Z]`).
[[nodiscard]]
constexpr bool is_ascii_alpha(char32_t c)
{
    // https://infra.spec.whatwg.org/#ascii-alpha
    return is_ascii_lower_alpha(c) || is_ascii_upper_alpha(c);
}

inline constexpr Charset256 is_ascii_alpha_set = detail::to_charset256(is_ascii_alpha);

inline constexpr Charset256 is_ascii_alphanumeric_set = is_ascii_alpha_set | is_ascii_digit_set;

[[nodiscard]]
constexpr bool is_ascii_alphanumeric(char8_t c)
{
    // https://infra.spec.whatwg.org/#ascii-alphanumeric
    return is_ascii_alphanumeric_set.contains(c);
}

[[nodiscard]]
constexpr bool is_ascii_alphanumeric(char32_t c)
{
    // https://infra.spec.whatwg.org/#ascii-alphanumeric
    return is_ascii(c) && is_ascii_alphanumeric(char8_t(c));
}

} // namespace ulight

#endif
