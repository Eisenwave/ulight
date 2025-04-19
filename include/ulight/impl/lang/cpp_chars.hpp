#ifndef ULIGHT_CPP_CHARS_HPP
#define ULIGHT_CPP_CHARS_HPP

#include "ulight/impl/ascii_chars.hpp"
#include "ulight/impl/chars.hpp"
#include "ulight/impl/unicode_chars.hpp"

namespace ulight {

inline constexpr Charset256 is_cpp_ascii_identifier_start_set
    = is_ascii_xid_start_set | detail::to_charset256(u8'_');

/// @brief Returns `true` iff `c` is in the set `[A-Za-z_]`.
[[nodiscard]]
constexpr bool is_cpp_ascii_identifier_start(char8_t c)
{
    return c == u8'_' || is_ascii_xid_start(c);
}

/// @brief Returns `true` iff `c` is in the set `[A-Za-z_]`.
[[nodiscard]]
constexpr bool is_cpp_ascii_identifier_start(char32_t c)
{
    return c == U'_' || is_ascii_xid_start(c);
}

constexpr bool is_cpp_identifier_start(char8_t c) = delete;

[[nodiscard]]
constexpr bool is_cpp_identifier_start(char32_t c)
{
    // https://eel.is/c++draft/lex.name#nt:identifier-start
    return c == U'_' || is_xid_start(c);
}

inline constexpr Charset256 is_cpp_ascii_identifier_continue_set = is_ascii_xid_continue_set;

/// @brief Returns `true` iff `c` is in the set `[A-Za-z0-9_]`.
[[nodiscard]]
constexpr bool is_cpp_ascii_identifier_continue(char8_t c)
{
    return is_ascii_xid_continue(c);
}

/// @brief Returns `true` iff `c` is in the set `[A-Za-z0-9_]`.
[[nodiscard]]
constexpr bool is_cpp_ascii_identifier_continue(char32_t c)
{
    return is_ascii_xid_continue(c);
}

constexpr bool is_cpp_identifier_continue(char8_t c) = delete;

[[nodiscard]]
constexpr bool is_cpp_identifier_continue(char32_t c)
{
    // https://eel.is/c++draft/lex.name#nt:identifier-start
    return c == U'_' || is_xid_continue(c);
}

inline constexpr Charset256 is_cpp_whitespace_set = detail::to_charset256(u8"\t\n\f\r \v");

/// @brief consistent with `isspace` in the C locale.
[[nodiscard]]
constexpr bool is_cpp_whitespace(char8_t c)
{
    return is_cpp_whitespace_set.contains(c);
}

/// @brief Consistent with `isspace` in the C locale.
[[nodiscard]]
constexpr bool is_cpp_whitespace(char32_t c)
{
    return is_ascii(c) && is_cpp_whitespace(char8_t(c));
}

inline constexpr Charset256 is_cpp_basic_set = is_ascii_alphanumeric_set
    | detail::to_charset256(u8"\t\v\f\r\n!\"#$%&'()*+,-./:;<>=?@[]\\^_`{|}~");

/// @brief Returns `true` iff `c` is in the
/// [basic character set](https://eel.is/c++draft/tab:lex.charset.basic).
[[nodiscard]]
constexpr bool is_cpp_basic(char8_t c)
{
    return is_cpp_basic_set.contains(c);
}

[[nodiscard]]
constexpr bool is_cpp_basic(char32_t c)
{
    return is_ascii(c) && is_cpp_basic(char8_t(c));
}

} // namespace ulight

#endif
