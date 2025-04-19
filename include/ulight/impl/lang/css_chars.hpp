#ifndef ULIGHT_CSS_CHARS_HPP
#define ULIGHT_CSS_CHARS_HPP

#include "ulight/impl/ascii_chars.hpp"
#include "ulight/impl/chars.hpp"
#include "ulight/impl/lang/html_chars.hpp"

namespace ulight {

// CSS =============================================================================================

[[nodiscard]]
constexpr bool is_css_newline(char8_t c)
{
    // https://www.w3.org/TR/css-syntax-3/#newline
    return c == u8'\n' || c == u8'\r' || c == u8'\f';
}

[[nodiscard]]
constexpr bool is_css_newline(char32_t c)
{
    // https://www.w3.org/TR/css-syntax-3/#newline
    return c == U'\n' || c == U'\r' || c == U'\f';
}

inline constexpr Charset256 is_css_newline_set = detail::to_charset256(is_css_newline);

[[nodiscard]]
constexpr bool is_css_whitespace(char8_t c)
{
    // https://www.w3.org/TR/css-syntax-3/#whitespace
    return is_html_whitespace(c);
}

[[nodiscard]]
constexpr bool is_css_whitespace(char32_t c)
{
    // https://www.w3.org/TR/css-syntax-3/#whitespace
    return is_html_whitespace(c);
}

inline constexpr Charset256 is_css_whitespace_set = is_html_whitespace_set;

inline constexpr Charset256 is_css_identifier_start_set
    = is_ascii_alpha_set | detail::to_charset256(u8'_') | ~is_ascii_set;

[[nodiscard]]
constexpr bool is_css_identifier_start(char8_t c)
{
    // https://www.w3.org/TR/css-syntax-3/#ident-start-code-point
    return is_css_identifier_start_set.contains(c);
}

[[nodiscard]]
constexpr bool is_css_identifier_start(char32_t c)
{
    // https://www.w3.org/TR/css-syntax-3/#ident-start-code-point
    return !is_ascii(c) || is_css_identifier_start(char8_t(c));
}

inline constexpr Charset256 is_css_identifier_set
    = is_css_identifier_start_set | is_ascii_digit_set | detail::to_charset256(u8'-');

[[nodiscard]]
constexpr bool is_css_identifier(char8_t c)
{
    // https://www.w3.org/TR/css-syntax-3/#ident-code-point
    return is_css_identifier_set.contains(c);
}

[[nodiscard]]
constexpr bool is_css_identifier(char32_t c)
{
    // https://www.w3.org/TR/css-syntax-3/#ident-code-point
    return !is_ascii(c) || is_css_identifier(char8_t(c));
}

} // namespace ulight

#endif
