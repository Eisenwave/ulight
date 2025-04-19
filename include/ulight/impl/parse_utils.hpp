#ifndef ULIGHT_PARSE_HPP
#define ULIGHT_PARSE_HPP

#include <cstddef>
#include <optional>
#include <string_view>

namespace ulight {

struct Blank_Line {
    std::size_t begin;
    std::size_t length;

    [[nodiscard]]
    constexpr explicit operator bool() const
    {
        return length != 0;
    }

    [[nodiscard]]
    friend constexpr bool operator==(Blank_Line, Blank_Line)
        = default;
};

/// @brief Returns a `Blank_Line` where `begin` is the index of the first whitespace character
/// that is part of the blank line sequence,
/// and where `length` is the length of the blank line sequence, in code units.
/// The last character in the sequence is always `\\n`.
///
/// Note that the terminating whitespace of the previous line
/// is not considered to be part of the blank line.
/// For example, in `"first\\n\\t\\t\\n\\n second"`,
/// the blank line sequence consists of `"\\t\\t\\n\\n"`.
[[nodiscard]]
Blank_Line find_blank_line_sequence(std::u8string_view str) noexcept;

struct Line_Result {
    /// @brief The length of the line contents, which is possibly zero.
    std::size_t content_length;
    /// @brief The length of the line terminator.
    /// This could be `0` for an unterminated line at the end of the file,
    /// `1` for an LF ending, or `2` for a CRLF ending.
    std::size_t terminator_length;

    [[nodiscard]]
    constexpr friend bool operator==(Line_Result, Line_Result)
        = default;
};

/// @brief Matches a line at the start of `str`.
/// A line is considered to be terminated by LF, CR, or CRLF.
[[nodiscard]]
Line_Result match_line(std::u8string_view str);

/// @brief Matches as many digits as possible, in a base of choice.
/// For bases above 10, lower and upper case characters are permitted.
/// @param str the string with digits at the beginning
/// @param base in range [2, 16]
/// @return The number of digits that belong to a numeric literal of the given base.
[[nodiscard]]
std::size_t match_digits(std::u8string_view str, int base = 10);

/// @brief Like `parse_integer_literal`, but does not permit negative numbers and results
/// in an unsigned integer.
/// @param str the string containing the prefix and literal digits
/// @return The parsed number.
[[nodiscard]]
std::optional<unsigned long long> parse_uinteger_literal(std::u8string_view str) noexcept;

/// @brief Converts a literal string to an signed integer.
/// The sign of the integer is based on a leading `-` character.
/// The base of the literal is automatically detected based on prefix:
/// - `0b` for binary
/// - `0` for octal
/// - `0x` for hexadecimal
/// - otherwise decimal
/// @param str the string containing the prefix and literal digits
/// @return The parsed number.
[[nodiscard]]
std::optional<long long> parse_integer_literal(std::u8string_view str) noexcept;

} // namespace ulight

#endif
