#ifndef ULIGHT_MMML_CHARS_HPP
#define ULIGHT_MMML_CHARS_HPP

#include "ulight/impl/chars.hpp"
#include "ulight/impl/lang/html_chars.hpp"

namespace ulight {

/// @brief Returns `true` if `c` is an escapable MMML character.
/// That is, if `\\c` would corresponds to the literal character `c`,
/// rather than starting a directive or being treated as literal text.
[[nodiscard]]
constexpr bool is_mmml_escapeable(char8_t c)
{
    return c == u8'\\' || c == u8'}' || c == u8'{';
}

[[nodiscard]]
constexpr bool is_mmml_escapeable(char32_t c)
{
    return c == U'\\' || c == U'}' || c == U'{';
}

inline constexpr Charset256 is_mmml_escapeable_set = detail::to_charset256(is_mmml_escapeable);

inline constexpr Charset256 is_mmml_special_character_set = detail::to_charset256(u8"{}\\[],");

[[nodiscard]]
constexpr bool is_mmml_special_character(char8_t c)
{
    return is_mmml_special_character_set.contains(c);
}

[[nodiscard]]
constexpr bool is_mmml_special_character(char32_t c)
{
    return is_ascii(c) && is_mmml_special_character(char8_t(c));
}

inline constexpr Charset256 is_mmml_ascii_argument_name_character_set
    = is_html_ascii_attribute_name_character_set - is_mmml_special_character_set;

[[nodiscard]]
constexpr bool is_mmml_ascii_argument_name_character(char8_t c)
{
    return is_mmml_ascii_argument_name_character_set.contains(c);
}

[[nodiscard]]
constexpr bool is_mmml_ascii_argument_name_character(char32_t c)
{
    return is_ascii(c) && is_mmml_ascii_argument_name_character(char8_t(c));
}

constexpr bool is_mmml_argument_name_character(char8_t c) = delete;

/// @brief Returns `true` if `c` can legally appear
/// in the name of an MMML directive argument.
[[nodiscard]]
constexpr bool is_mmml_argument_name_character(char32_t c)
{
    return !is_mmml_special_character(c) && is_html_attribute_name_character(c);
}

inline constexpr Charset256 is_mmml_ascii_directive_name_character_set
    = is_html_ascii_tag_name_character_set;

[[nodiscard]]
constexpr bool is_mmml_ascii_directive_name_character(char8_t c)
{
    return is_html_ascii_tag_name_character(c);
}

[[nodiscard]]
constexpr bool is_mmml_ascii_directive_name_character(char32_t c)
{
    return is_ascii(c) && is_html_ascii_tag_name_character(char8_t(c));
}

constexpr bool is_mmml_directive_name_character(char8_t) = delete;

/// @brief Returns `true` if `c` can legally appear
/// in the name of an MMML directive.
[[nodiscard]]
constexpr bool is_mmml_directive_name_character(char32_t c)
{
    return is_html_tag_name_character(c);
}

} // namespace ulight

#endif
