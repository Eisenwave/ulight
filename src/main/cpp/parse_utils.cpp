#include <algorithm>
#include <charconv>
#include <cstddef>
#include <optional>
#include <string_view>
#include <system_error>

#include "ulight/impl/assert.hpp"
#include "ulight/ulight.hpp"

#include "ulight/impl/parse_utils.hpp"
#include "ulight/impl/strings.hpp"

#include "ulight/impl/lang/html_chars.hpp"

namespace ulight {

Blank_Line find_blank_line_sequence // NOLINT(bugprone-exception-escape)
    (std::u8string_view str) noexcept
{
    enum struct State : Underlying {
        /// @brief We are at the start of a line.
        maybe_blank,
        /// @brief There are non-whitespace characters on this line.
        not_blank,
        /// @brief At least one line is blank.
        blank,
    };
    State state = State::maybe_blank;

    std::size_t blank_begin = 0;
    std::size_t blank_end;

    for (std::size_t i = 0; i < str.size(); ++i) {
        switch (state) {
        case State::maybe_blank: {
            if (str[i] == u8'\n') {
                state = State::blank;
                blank_end = i + 1;
            }
            else if (!is_html_whitespace(str[i])) {
                state = State::not_blank;
            }
            continue;
        }
        case State::not_blank: {
            if (str[i] == u8'\n') {
                state = State::maybe_blank;
                blank_begin = i + 1;
            }
            continue;
        }
        case State::blank: {
            if (str[i] == u8'\n') {
                blank_end = i + 1;
            }
            else if (!is_html_whitespace(str[i])) {
                return { .begin = blank_begin, .length = blank_end - blank_begin };
            }
            continue;
        }
        }
        ULIGHT_ASSERT_UNREACHABLE(u8"Invalid state");
    }

    static_assert(!Blank_Line {}, "A value-initialized Blank_Line should be falsy");
    if (state == State::blank) {
        return { .begin = blank_begin, .length = str.size() - blank_begin };
    }
    return {};
}

namespace {

std::optional<unsigned long long> parse_uinteger_digits(std::u8string_view text, int base)
{
    const auto sv = as_string_view(text);
    unsigned long long value;
    const std::from_chars_result result
        = std::from_chars(sv.data(), sv.data() + sv.size(), value, base);
    if (result.ec != std::errc {}) {
        return {};
    }
    return value;
}

} // namespace

Line_Result match_line(std::u8string_view str)
{
    const std::size_t length = str.find_first_of(u8"\r\n");
    if (length == std::u8string_view::npos) {
        return { .content_length = length, .terminator_length = 0 };
    }
    const std::size_t terminator_length
        = str[length] == u8'\r' && length + 1 < str.length() && str[length + 1] == u8'\n' ? 2 : 1;
    return { .content_length = length, .terminator_length = terminator_length };
}

std::size_t match_digits(std::u8string_view str, int base)
{
    ULIGHT_ASSERT((base >= 2 && base <= 10) || base == 16);
    static constexpr std::u8string_view hexadecimal_digits = u8"0123456789abcdefABCDEF";

    const std::u8string_view digits
        = base == 16 ? hexadecimal_digits : hexadecimal_digits.substr(0, std::size_t(base));

    // std::min covers the case of std::u8string_view::npos
    return std::min(str.find_first_not_of(digits), str.size());
}

std::optional<unsigned long long> parse_uinteger_literal(std::u8string_view str) noexcept
{
    if (str.empty()) {
        return {};
    }
    if (str.starts_with(u8"0b")) {
        return parse_uinteger_digits(str.substr(2), 2);
    }
    if (str.starts_with(u8"0x")) {
        return parse_uinteger_digits(str.substr(2), 16);
    }
    if (str.starts_with(u8'0')) {
        return parse_uinteger_digits(str, 8);
    }
    return parse_uinteger_digits(str, 10);
}

std::optional<long long> parse_integer_literal(std::u8string_view str) noexcept
{
    if (str.empty()) {
        return std::nullopt;
    }
    if (str.starts_with(u8'-')) {
        if (auto positive = parse_uinteger_literal(str.substr(1))) {
            // Negating as Uint is intentional and prevents overflow.
            return static_cast<long long>(-*positive);
        }
        return std::nullopt;
    }
    if (auto result = parse_uinteger_literal(str)) {
        return static_cast<long long>(*result);
    }
    return std::nullopt;
}

} // namespace ulight
