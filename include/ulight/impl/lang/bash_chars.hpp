#ifndef ULIGHT_BASH_CHARS_HPP
#define ULIGHT_BASH_CHARS_HPP

#include "ulight/impl/ascii_chars.hpp"
#include "ulight/impl/chars.hpp"
#include "ulight/impl/charset.hpp"

namespace ulight {

inline constexpr Charset256 is_bash_whitespace_set = detail::to_charset256(u8" \t\v\r\n");

[[nodiscard]]
constexpr bool is_bash_whitespace(char8_t c)
{
    return is_bash_whitespace_set.contains(c);
}

[[nodiscard]]
constexpr bool is_bash_whitespace(char32_t c)
{
    return is_ascii(c) && is_bash_whitespace(char8_t(c));
}

[[nodiscard]]
constexpr bool is_bash_blank(char8_t c)
{
    // https://www.gnu.org/software/bash/manual/bash.html#Definitions-1
    return c == u8' ' || c == u8'\t';
}

[[nodiscard]]
constexpr bool is_bash_blank(char32_t c)
{
    // https://www.gnu.org/software/bash/manual/bash.html#Definitions-1
    return c == U' ' || c == U'\t';
}

inline constexpr Charset256 is_bash_blank_set = detail::to_charset256(u8" \t");

inline constexpr Charset256 is_bash_metacharacter_set
    = is_bash_blank_set | detail::to_charset256(u8"|&;()<>");

[[nodiscard]]
constexpr bool is_bash_metacharacter(char8_t c)
{
    // https://www.gnu.org/software/bash/manual/bash.html#index-metacharacter
    return is_bash_metacharacter_set.contains(c);
}

[[nodiscard]]
constexpr bool is_bash_metacharacter(char32_t c)
{
    // https://www.gnu.org/software/bash/manual/bash.html#index-metacharacter
    return is_ascii(c) && is_bash_metacharacter(char8_t(c));
}

inline constexpr Charset256 is_bash_escapable_in_double_quotes_set
    = detail::to_charset256(u8"'$`\"\\\n");

/// @brief Returns `true` iff `c` is a character
/// for which a preceding backslash within a double-quoted sring
/// retains its special meaning.
///
/// For example, `\x` has no special meaning within double quotes (outside it does),
/// but
[[nodiscard]]
constexpr bool is_bash_escapable_in_double_quotes(char8_t c)
{
    // https://www.gnu.org/software/bash/manual/bash.html#Double-Quotes
    return is_bash_escapable_in_double_quotes_set.contains(c);
}

[[nodiscard]]
constexpr bool is_bash_escapable_in_double_quotes(char32_t c)
{
    // https://www.gnu.org/software/bash/manual/bash.html#Double-Quotes
    return is_ascii(c) && is_bash_escapable_in_double_quotes(char8_t(c));
}

inline constexpr Charset256 is_bash_special_parameter_set = detail::to_charset256(u8"*@#?-$!0");

/// @brief Returns `true` iff `c` forms a parameter substitution
/// when preceded by `$` despite not being a variable name.
///
/// For example, this is `true` for `'#'`
/// because `$#` expands to the number of positional parameters.
[[nodiscard]]
constexpr bool is_bash_special_parameter(char8_t c)
{
    // https://www.gnu.org/software/bash/manual/bash.html#Special-Parameters
    return is_bash_special_parameter_set.contains(c);
}

[[nodiscard]]
constexpr bool is_bash_special_parameter(char32_t c)
{
    // https://www.gnu.org/software/bash/manual/bash.html#Special-Parameters
    return is_ascii(c) && is_bash_special_parameter(char8_t(c));
}

[[nodiscard]]
constexpr bool is_bash_identifier_start(char8_t c)
{
    // https://www.gnu.org/software/bash/manual/bash.html#index-name
    return is_ascii_alpha(c) || c == u8'_';
}

[[nodiscard]]
constexpr bool is_bash_identifier_start(char32_t c)
{
    // https://www.gnu.org/software/bash/manual/bash.html#index-name
    return is_ascii_alpha(c) || c == U'_';
}

inline constexpr Charset256 is_bash_identifier_start_set
    = detail::to_charset256(is_bash_identifier_start);

[[nodiscard]]
constexpr bool is_bash_identifier(char8_t c)
{
    // https://www.gnu.org/software/bash/manual/bash.html#index-name
    return is_ascii_alphanumeric(c) || c == u8'_';
}

[[nodiscard]]
constexpr bool is_bash_identifier(char32_t c)
{
    // https://www.gnu.org/software/bash/manual/bash.html#index-name
    return is_ascii_alphanumeric(c) || c == U'_';
}

inline constexpr Charset256 is_bash_identifier_set = detail::to_charset256(is_bash_identifier);

inline constexpr Charset256 is_bash_parameter_substitution_start_set
    = detail::to_charset256(u8"({") | is_bash_identifier_start_set | is_bash_special_parameter_set;

/// @brief Returns `true` iff `c` would start a parameter substitution when following `'$'`.
[[nodiscard]]
constexpr bool is_bash_parameter_substitution_start(char8_t c)
{
    return is_bash_parameter_substitution_start_set.contains(c);
}

constexpr bool is_bash_parameter_substitution_start(char32_t c)
{
    return is_ascii(c) && is_bash_parameter_substitution_start(char8_t(c));
}

inline constexpr Charset256 is_bash_unquoted_terminator_set
    = detail::to_charset256(u8"\\'\"") | is_bash_whitespace_set | is_bash_metacharacter_set;

/// @brief Returns `true` if `c` is a character that ends a sequence of unquoted characters that
/// comprise a single argument for the highlighter.
///
/// Any characters not in this set would form a contiguous word.
/// Notably, `/` and `.` are not in this set, so `./path/to` form a word,
/// and are highlighted as one token.
[[nodiscard]]
constexpr bool is_bash_unquoted_terminator(char8_t c)
{
    return is_bash_unquoted_terminator_set.contains(c);
}

[[nodiscard]]
constexpr bool is_bash_unquoted_terminator(char32_t c)
{
    return is_ascii(c) && is_bash_unquoted_terminator(char8_t(c));
}

} // namespace ulight

#endif
