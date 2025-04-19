#ifndef ULIGHT_HTML_CHARS_HPP
#define ULIGHT_HTML_CHARS_HPP

#include "ulight/impl/ascii_chars.hpp"
#include "ulight/impl/chars.hpp"
#include "ulight/impl/unicode_chars.hpp"

namespace ulight {

// HTML ============================================================================================

inline constexpr Charset256 is_html_ascii_tag_name_character_set
    = is_ascii_alphanumeric_set | detail::to_charset256(u8"-._");

/// @brief Returns `true` if `c` is an ASCII character
/// that can legally appear in the name of an HTML tag.
[[nodiscard]]
constexpr bool is_html_ascii_tag_name_character(char8_t c)
{
    return is_html_ascii_tag_name_character_set.contains(c);
}

[[nodiscard]]
constexpr bool is_html_ascii_control(char8_t c)
{
    // https://infra.spec.whatwg.org/#control
    return c <= u8'\u001F' || c == u8'\N{DELETE}';
}

inline constexpr Charset256 is_html_ascii_control_set
    = detail::to_charset256(is_html_ascii_control);

constexpr bool is_html_control(char8_t c) = delete;

[[nodiscard]]
constexpr bool is_html_control(char32_t c)
{
    // https://infra.spec.whatwg.org/#control
    return c <= U'\u001F' || (c >= U'\u007F' && c <= U'\u009F');
}

constexpr bool is_html_tag_name_character(char8_t c) = delete;

[[nodiscard]]
constexpr bool is_html_tag_name_character(char32_t c)
{
    // https://html.spec.whatwg.org/dev/syntax.html#syntax-tag-name
    // https://html.spec.whatwg.org/dev/custom-elements.html#valid-custom-element-name
    // Note that the EBNF grammar on the page above does not include upper-case characters.
    // That is simply because HTML is case-insensitive,
    // not because upper-case tag names are malformed.
    // We accept them here.

    // clang-format off
    return is_ascii_alphanumeric(c)
        ||  c == U'-'
        ||  c == U'.'
        ||  c == U'_'
        ||  c == U'\N{MIDDLE DOT}'
        || (c >= U'\u00C0' && c <= U'\u00D6')
        || (c >= U'\u00D8' && c <= U'\u00F6')
        || (c >= U'\u00F8' && c <= U'\u037D')
        || (c >= U'\u037F' && c <= U'\u1FFF')
        || (c >= U'\u200C' && c <= U'\u200D')
        || (c >= U'\u203F' && c <= U'\u2040')
        || (c >= U'\u2070' && c <= U'\u218F')
        || (c >= U'\u2C00' && c <= U'\u2FEF')
        || (c >= U'\u3001' && c <= U'\uD7FF')
        || (c >= U'\uF900' && c <= U'\uFDCF')
        || (c >= U'\uFDF0' && c <= U'\uFFFD')
        || (c >= U'\U00010000' && c <= U'\U000EFFFF');
    // clang-format on
}

inline constexpr Charset256 is_html_whitespace_set = detail::to_charset256(u8" \t\n\f\r");

/// @brief Returns `true` if `c` is whitespace.
/// Note that "whitespace" matches the HTML standard definition here,
/// and unlike the C locale,
/// vertical tabs are not included.
[[nodiscard]]
constexpr bool is_html_whitespace(char8_t c)
{
    // https://infra.spec.whatwg.org/#ascii-whitespace
    return is_html_whitespace_set.contains(c);
}

/// @brief Returns `true` if `c` is whitespace.
/// Note that "whitespace" matches the HTML standard definition here,
/// and unlike the C locale,
/// vertical tabs are not included.
[[nodiscard]]
constexpr bool is_html_whitespace(char32_t c)
{
    // https://infra.spec.whatwg.org/#ascii-whitespace
    return is_ascii(c) && is_html_whitespace(char8_t(c));
}

inline constexpr Charset256 is_html_ascii_attribute_name_character_set
    = is_ascii_set - is_html_ascii_control_set - detail::to_charset256(u8" \"'>/=");

[[nodiscard]]
constexpr bool is_html_ascii_attribute_name_character(char8_t c)
{
    // https://html.spec.whatwg.org/dev/syntax.html#syntax-attribute-name
    return is_html_ascii_attribute_name_character_set.contains(c);
}

[[nodiscard]]
constexpr bool is_html_ascii_attribute_name_character(char32_t c)
{
    // https://html.spec.whatwg.org/dev/syntax.html#syntax-attribute-name
    return is_ascii(c) && is_html_ascii_attribute_name_character(char8_t(c));
}

constexpr bool is_html_attribute_name_character(char8_t c) = delete;

[[nodiscard]]
constexpr bool is_html_attribute_name_character(char32_t c)
{
    // https://html.spec.whatwg.org/dev/syntax.html#syntax-attribute-name
    return is_ascii(c) ? is_html_ascii_attribute_name_character(c) : !is_noncharacter(c);
}

inline constexpr Charset256 is_html_unquoted_attribute_value_terminator_set
    = is_html_whitespace_set | detail::to_charset256(u8"\"'=<>`");

/// @brief Returns `true` iff `c` HTML whitespace or one of the special characters that terminates
/// unquoted attribute values.
[[nodiscard]]
constexpr bool is_html_unquoted_attribute_value_terminator(char8_t c)
{
    // https://html.spec.whatwg.org/dev/syntax.html#unquoted
    return is_html_unquoted_attribute_value_terminator_set.contains(c);
}

inline constexpr Charset256 is_html_ascii_unquoted_attribute_value_character_set
    = is_ascii_set - is_html_unquoted_attribute_value_terminator_set;

/// @brief Returns `true` if `c` can appear in an attribute value string with no
/// surrounding quotes, such as in `<h2 id=heading>`.
///
/// Note that the HTML standard also restricts that character references must be unambiguous,
/// but this function has no way of verifying that.
[[nodiscard]]
constexpr bool is_html_ascii_unquoted_attribute_value_character(char8_t c)
{
    // https://html.spec.whatwg.org/dev/syntax.html#unquoted
    return is_html_ascii_unquoted_attribute_value_character_set.contains(c);
}

[[nodiscard]]
constexpr bool is_html_ascii_unquoted_attribute_value_character(char32_t c)
{
    return is_ascii(c) && is_html_ascii_unquoted_attribute_value_character(char8_t(c));
}

constexpr bool is_html_unquoted_attribute_value_character(char8_t c) = delete;

/// @brief Returns `true` if `c` can appear in an attribute value string with no
/// surrounding quotes, such as in `<h2 id=heading>`.
///
/// Note that the HTML standard also restricts that character references must be unambiguous,
/// but this function has no way of verifying that.
[[nodiscard]]
constexpr bool is_html_unquoted_attribute_value_character(char32_t c)
{
    // https://html.spec.whatwg.org/dev/syntax.html#unquoted
    return !is_ascii(c) || is_html_ascii_unquoted_attribute_value_character(char8_t(c));
}

/// @brief Returns `true` iff `c`
/// is part of the minimal set of characters
/// so that text comprised of such characters
/// can be passed through into raw HTML text,
/// without its meaning altered.
///
/// Specifically, `c` cannot be `'<'` or `'&'`
/// because these could initiate an HTML tag or entity.
[[nodiscard]]
constexpr bool is_html_min_raw_passthrough_character(char8_t c)
{
    return c != u8'<' && c != u8'&';
}

/// @brief Returns `true` iff `c`
/// is part of the minimal set of characters
/// so that text comprised of such characters
/// can be passed through into raw HTML text,
/// without its meaning altered.
///
/// Specifically, `c` cannot be `'<'` or `'&'`
/// because these could initiate an HTML tag or entity.
[[nodiscard]]
constexpr bool is_html_min_raw_passthrough_character(char32_t c)
{
    return c != U'<' && c != U'&';
}

inline constexpr Charset256 is_html_min_raw_passthrough_character_set
    = detail::to_charset256(is_html_min_raw_passthrough_character);

} // namespace ulight

#endif
