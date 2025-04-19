#ifndef ULIGHT_JS_CHARS_HPP
#define ULIGHT_JS_CHARS_HPP

#include "ulight/impl/ascii_chars.hpp"

namespace ulight {

[[nodiscard]]
constexpr bool is_js_whitespace(char8_t c)
    = delete;

/// @brief Classifications are from
/// https://262.ecma-international.org/15.0/index.html#sec-grammar-summary
// Note that this should be updated whenever the Unicode standard changes.
// Currently, the Unicode characters are not fully supported.
[[nodiscard]]
constexpr bool is_js_whitespace(char32_t c)
{
    // https://262.ecma-international.org/15.0/index.html#sec-white-space
    // clang-format off
        return c == U' '
            || c == U'\t'
            || c == U'\v'
            || c == U'\f'
            || c == U'\n'
            || c == U'\r'
            || c == U'\N{NO-BREAK SPACE}'
            || c == U'\N{LINE SEPARATOR}'
            || c == U'\N{PARAGRAPH SEPARATOR}'
            || c == U'\N{ZERO WIDTH NO-BREAK SPACE}'
            || c == U'\N{NARROW NO-BREAK SPACE}'
            || c == U'\N{MEDIUM MATHEMATICAL SPACE}'
            || c == U'\N{IDEOGRAPHIC SPACE}'
            || (c >= U'\u1680' && c <= U'\u180E')
            || (c >= U'\u2000' && c <= U'\u200A');
    // clang-format on
}

[[nodiscard]]
constexpr bool is_js_identifier_start(char8_t c)
    = delete;

[[nodiscard]]
constexpr bool is_js_identifier_start(char32_t c)
{
    // https://262.ecma-international.org/15.0/index.html#prod-IdentifierName
    // clang-format off
        return c == U'$'
            || c == U'_' // Special JS characters.
            || is_ascii_alpha(c)
            || (c >= U'\u00C0' && c <= U'\u00D6') // Lu, Ll, Lt, Lm, Lo, Nl categories.
            || (c >= U'\u00D8' && c <= U'\u00F6')
            || (c >= U'\u00F8' && c <= U'\u02FF')
            || (c >= U'\u0370' && c <= U'\u037D')
            || (c >= U'\u037F' && c <= U'\u1FFF')
            || (c >= U'\u200C' && c <= U'\u200D')
            || (c >= U'\u2070' && c <= U'\u218F')
            || (c >= U'\u2C00' && c <= U'\u2FEF')
            || (c >= U'\u3001' && c <= U'\uD7FF')
            || (c >= U'\uF900' && c <= U'\uFDCF')
            || (c >= U'\uFDF0' && c <= U'\uFFFD')
            || (c >= U'\U00010000' && c <= U'\U000EFFFF');
    // clang-format on
}

[[nodiscard]]
constexpr bool is_js_identifier_part(char8_t c)
    = delete;

[[nodiscard]]
constexpr bool is_js_identifier_part(char32_t c)
{
    // https://262.ecma-international.org/15.0/index.html#prod-IdentifierPart
    // clang-format off
        return is_js_identifier_start(c)
            || is_ascii_digit(c)
            || (c >= U'\u0300' && c <= U'\u036F') // // Mn, Mc, Nd, Pc categories.
            || (c >= U'\u1DC0' && c <= U'\u1DFF')
            || (c >= U'\u20D0' && c <= U'\u20FF')
            || (c >= U'\uFE20' && c <= U'\uFE2F')
            || (c >= U'\u0660' && c <= U'\u0669') // Arabic-Indic digits.
            || (c >= U'\u06F0' && c <= U'\u06F9') // Extended Arabic-Indic digits.
            || (c >= U'\u07C0' && c <= U'\u07C9') // NKo digits.
            || (c >= U'\u0966' && c <= U'\u096F') // Devanagari digits.
            || c == U'\u200C'  // Zero-width joiner.
            || c == U'\u200D'; // Joiner.
    // clang-format on
}

[[nodiscard]]
constexpr bool is_jsx_tag_name_part(char8_t c)
    = delete;

// JSX tag names can include identifiers, hyphens, colons, and periods
// according to the official JSX spec.
[[nodiscard]]
constexpr bool is_jsx_tag_name_part(char32_t c)
{
    // https://facebook.github.io/jsx/#prod-JSXElementName
    return is_js_identifier_part(c) || c == U'-' || c == U':' || c == U'.';
}

} // namespace ulight

#endif
