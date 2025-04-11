#include <algorithm>
#include <bitset>
#include <cstddef>
#include <memory_resource>
#include <optional>
#include <string_view>
#include <vector>

#include "ulight/impl/buffer.hpp"
#include "ulight/impl/highlight.hpp"
#include "ulight/ulight.hpp"

#include "ulight/impl/chars.hpp"
#include "ulight/impl/lua.hpp"
#include "ulight/impl/unicode.hpp"

namespace ulight {

namespace lua {

#define ULIGHT_LUA_TOKEN_TYPE_U8_CODE(id, code, highlight, strict) u8##code,
#define ULIGHT_LUA_TOKEN_TYPE_LENGTH(id, code, highlight, strict) (sizeof(u8##code) - 1),
#define ULIGHT_LUA_TOKEN_HIGHLIGHT_TYPE(id, code, highlight, strict) (Highlight_Type::highlight),
#define ULIGHT_LUA_TOKEN_TYPE_STRICT_STRING(id, code, highlight, strict) #strict

namespace {

inline constexpr std::u8string_view token_type_codes[] {
    ULIGHT_LUA_TOKEN_ENUM_DATA(ULIGHT_LUA_TOKEN_TYPE_U8_CODE)
};

static_assert(std::ranges::is_sorted(token_type_codes));

inline constexpr unsigned char token_type_lengths[] {
    ULIGHT_LUA_TOKEN_ENUM_DATA(ULIGHT_LUA_TOKEN_TYPE_LENGTH)
};

inline constexpr Highlight_Type token_type_highlights[] {
    ULIGHT_LUA_TOKEN_ENUM_DATA(ULIGHT_LUA_TOKEN_HIGHLIGHT_TYPE)
};

inline constexpr std::bitset<lua_token_type_count> token_type_strictness {
    ULIGHT_LUA_TOKEN_ENUM_DATA(ULIGHT_LUA_TOKEN_TYPE_STRICT_STRING), lua_token_type_count
};

} // namespace

/// @brief Returns the in-code representation of `type`.
[[nodiscard]]
std::u8string_view lua_token_type_code(Lua_Token_Type type) noexcept
{
    return token_type_codes[std::size_t(type)];
}

/// @brief Equivalent to `lua_token_type_code(type).length()`.
[[nodiscard]]
std::size_t lua_token_type_length(Lua_Token_Type type) noexcept
{
    return token_type_lengths[std::size_t(type)];
}

[[nodiscard]]
Highlight_Type lua_token_type_highlight(Lua_Token_Type type) noexcept
{
    return token_type_highlights[std::size_t(type)];
}

[[nodiscard]]
bool lua_token_type_is_strict(Lua_Token_Type type) noexcept
{
    return token_type_strictness[std::size_t(type)];
}

[[nodiscard]]
std::optional<Lua_Token_Type> lua_token_type_by_code(std::u8string_view code) noexcept
{
    const std::u8string_view* const result = std::ranges::lower_bound(token_type_codes, code);
    if (result == std::end(token_type_codes) || *result != code) {
        return {};
    }
    return Lua_Token_Type(result - token_type_codes);
}

std::size_t match_whitespace(std::u8string_view str)
{
    constexpr auto predicate = [](char8_t c) { return is_lua_whitespace(c); };
    return std::size_t(std::ranges::find_if_not(str, predicate) - str.begin());
}

std::size_t match_non_whitespace(std::u8string_view str)
{
    constexpr auto predicate = [](char8_t c) { return !is_lua_whitespace(c); };
    return std::size_t(std::ranges::find_if_not(str, predicate) - str.begin());
}

std::size_t match_line_comment(std::u8string_view s) noexcept
{
    if (!s.starts_with(u8"--")) {
        return 0;
    }

    // Skip the -- prefix
    std::size_t length = 2;

    // If it's not a block comment (--[[), then it's a line comment
    if (s.length() >= 4 && s.substr(0, 4) == u8"--[[") {
        return 0;
    }

    // Check for long comment syntax with equals (--[=[ ]=])
    if (s.length() >= 3 && s[2] == u8'[') {
        std::size_t idx = 3;

        // Count equals signs
        while (idx < s.length() && s[idx] == u8'=') {
            idx++;
        }

        // If we have a well-formed opening [, it's a block comment, not a line comment.
        if (idx < s.length() && s[idx] == u8'[') {
            return 0;
        }
    }

    // Continue until EoL
    while (length < s.length()) {
        if (s[length] == u8'\n') {
            return length;
        }
        ++length;
    }

    return length;
}

Comment_Result match_block_comment(std::u8string_view s) noexcept
{
    // Check for --[[ style comment
    if (s.length() < 4 || !s.starts_with(u8"--[[")) {
        // Check for --[=[ style comment with equals signs
        if (s.length() < 5 || !s.starts_with(u8"--[")) {
            return {};
        }

        std::size_t eq_count = 0;
        std::size_t idx = 3;

        // Count equals signs
        while (idx < s.length() && s[idx] == u8'=') {
            eq_count++;
            idx++;
        }

        // We need at least one equals sign and an opening bracket
        if (eq_count == 0 || idx >= s.length() || s[idx] != u8'[') {
            return {};
        }

        // Now find the matching closing delimiter
        std::size_t closing_idx = idx + 1;
        while (closing_idx < s.length()) {
            // Look for ]={eq_count}]
            if (s[closing_idx] == u8']') {
                bool found = true;
                for (std::size_t i = 0; i < eq_count; i++) {
                    if (closing_idx + 1 + i >= s.length() || s[closing_idx + 1 + i] != u8'=') {
                        found = false;
                        break;
                    }
                }

                if (found && closing_idx + 1 + eq_count < s.length()
                    && s[closing_idx + 1 + eq_count] == u8']') {
                    return Comment_Result { .length = closing_idx + 2 + eq_count,
                                            .is_terminated = true };
                }
            }
            closing_idx++;
        }

        return Comment_Result { .length = s.length(), .is_terminated = false };
    }

    // Find the matching closing ]].
    std::size_t closing_idx = 4;
    while (closing_idx < s.length() - 1) {
        if (s[closing_idx] == u8']' && s[closing_idx + 1] == u8']') {
            return Comment_Result { .length = closing_idx + 2, .is_terminated = true };
        }
        closing_idx++;
    }

    // No match.
    return Comment_Result { .length = s.length(), .is_terminated = false };
}

String_Literal_Result match_string_literal(std::u8string_view str)
{
    if (str.empty()) {
        return {};
    }

    // Quoted strings; single or double quotes
    if (str[0] == u8'"' || str[0] == u8'\'') {
        const char8_t quote = str[0];
        std::size_t length = 1;
        bool escaped = false;

        while (length < str.length()) {
            const char8_t c = str[length];

            if (escaped) {
                escaped = false;
            }
            else if (c == u8'\\') {
                escaped = true;
            }
            else if (c == quote) {
                return String_Literal_Result { .length = length + 1,
                                               .is_long_string = false,
                                               .terminated = true };
            }
            else if (c == u8'\n') {
                // Unterminated.
                return String_Literal_Result { .length = length,
                                               .is_long_string = false,
                                               .terminated = false };
            }

            length++;
        }

        // Unterminated.
        return String_Literal_Result { .length = length,
                                       .is_long_string = false,
                                       .terminated = false };
    }

    // Long bracket strings [[ ]] or [=[ ]=].
    if (str.starts_with(u8"[[")) {
        std::size_t length = 2;

        while (length < str.length() - 1) {
            if (str[length] == u8']' && str[length + 1] == u8']') {
                return String_Literal_Result { .length = length + 2,
                                               .is_long_string = true,
                                               .terminated = true };
            }
            length++;
        }

        // Unterminated long string.
        return String_Literal_Result { .length = str.length(),
                                       .is_long_string = true,
                                       .terminated = false };
    }

    // [=[ ]=] strings with equals signs.
    if (str.starts_with(u8"[") && str.length() >= 3) {
        std::size_t eq_count = 0;
        std::size_t idx = 1;

        while (idx < str.length() && str[idx] == u8'=') {
            eq_count++;
            idx++;
        }

        if (eq_count == 0 || idx >= str.length() || str[idx] != u8'[') {
            return {};
        }

        std::size_t closing_idx = idx + 1;
        while (closing_idx < str.length()) {
            // Look for ]={eq_count}].
            if (str[closing_idx] == u8']') {
                bool found = true;
                for (std::size_t i = 0; i < eq_count; i++) {
                    if (closing_idx + 1 + i >= str.length() || str[closing_idx + 1 + i] != u8'=') {
                        found = false;
                        break;
                    }
                }

                if (found && closing_idx + 1 + eq_count < str.length()
                    && str[closing_idx + 1 + eq_count] == u8']') {
                    return String_Literal_Result { .length = closing_idx + 2 + eq_count,
                                                   .is_long_string = true,
                                                   .terminated = true };
                }
            }
            closing_idx++;
        }

        // No matches.
        return String_Literal_Result { .length = str.length(),
                                       .is_long_string = true,
                                       .terminated = false };
    }

    return {};
}

std::size_t match_number(std::u8string_view str)
{
    if (str.empty()) {
        return 0;
    }

    std::size_t length = 0;

    // Hex (0x...).
    if (str.length() >= 2 && str[0] == u8'0' && (str[1] == u8'x' || str[1] == u8'X')) {
        length = 2;

        while (length < str.length() && is_ascii_hex_digit(str[length])) {
            length++;
        }

        // Fractions e.g. (0x1.Fp10).
        if (length < str.length() && str[length] == u8'.') {
            length++;
            while (length < str.length() && is_ascii_hex_digit(str[length])) {
                length++;
            }
        }

        // Exponent (0x1p10)
        if (length < str.length() && (str[length] == u8'p' || str[length] == u8'P')) {
            length++;

            // Optional sign.
            if (length < str.length() && (str[length] == u8'+' || str[length] == u8'-')) {
                length++;
            }
            while (length < str.length() && is_ascii_digit(str[length])) {
                length++;
            }
        }

        return length;
    }

    // Leading digits.
    while (length < str.length() && is_ascii_digit(str[length])) {
        length++;
    }

    if (length == 0 && str.length() >= 2 && str[0] == u8'.' && is_ascii_digit(str[1])) {
        length = 1;
        while (length < str.length() && is_ascii_digit(str[length])) {
            length++;
        }
    }
    else if (length > 0) {
        // Decimal point and fractional.
        if (length < str.length() && str[length] == u8'.') {
            length++;
            while (length < str.length() && is_ascii_digit(str[length])) {
                length++;
            }
        }
    }

    if (length == 0 || (length == 1 && str[0] == u8'.')) {
        return 0;
    }

    // Exponent (1e10, 1.5e-10).
    if (length < str.length() && (str[length] == u8'e' || str[length] == u8'E')) {
        const std::size_t exp_start = length;
        length++;

        // Optional sign.
        if (length < str.length() && (str[length] == u8'+' || str[length] == u8'-')) {
            length++;
        }

        // Exponent digits.
        std::size_t exp_digits = 0;
        while (length < str.length() && is_ascii_digit(str[length])) {
            length++;
            exp_digits++;
        }

        // Revert back if found none.
        if (exp_digits == 0) {
            length = exp_start;
        }
    }

    return length;
}

std::size_t match_identifier(std::u8string_view str)
{
    if (str.empty()) {
        return 0;
    }

    // Check first character.
    const auto [first_char, first_units] = utf8::decode_and_length_or_throw(str);
    if (!is_lua_identifier_start(first_char)) {
        return 0;
    }

    auto length = static_cast<std::size_t>(first_units);

    // Check rest of the characters.
    while (length < str.length()) {
        const auto [code_point, units] = utf8::decode_and_length_or_throw(str.substr(length));
        if (!is_lua_identifier_continue(code_point)) {
            break;
        }
        length += static_cast<std::size_t>(units);
    }

    return length;
}

std::optional<Lua_Token_Type> match_operator_or_punctuation(std::u8string_view str)
{
    using enum Lua_Token_Type;

    if (str.empty()) {
        return {};
    }

    switch (str[0]) {
    case u8'+': return plus;
    case u8'-': return minus;
    case u8'*': return asterisk;

    case u8'/':
        if (str.length() > 1 && str[1] == u8'/') {
            return floor_div;
        }
        return slash;

    case u8'%': return percent;

    case u8'^': return caret;

    case u8'#': return hash;

    case u8'=':
        if (str.length() > 1 && str[1] == u8'=') {
            return eq_eq;
        }
        return eq;

    case u8'<':
        if (str.length() > 1) {
            if (str[1] == u8'=') {
                return less_eq;
            }
            if (str[1] == u8'<') {
                return left_shift;
            }
            // <const> is handled separately in the `highlight_lua` function.
            if (str.length() >= 6 && str.substr(1, 5) == u8"const>") {
                return {}; // Return empty for <const> so it's handled by the special case
            }
        }
        return less;

    case u8'>':
        if (str.length() > 1) {
            if (str[1] == u8'=') {
                return greater_eq;
            }
            if (str[1] == u8'>') {
                return right_shift;
            }
        }
        return greater;

    case u8'~':
        if (str.length() > 1 && str[1] == u8'=') {
            return tilde_eq;
        }
        return tilde;

    case u8'&': return amp;

    case u8'|': return pipe;

    case u8'.':
        if (str.length() > 2 && str[1] == u8'.' && str[2] == u8'.') {
            return dot_dot_dot;
        }
        if (str.length() > 1 && str[1] == u8'.') {
            return dot_dot;
        }
        return dot;

    case u8':':
        if (str.length() > 1 && str[1] == u8':') {
            return colon_colon;
        }
        return colon;

    case u8';': return semicolon;

    case u8',': return comma;

    case u8'(': return left_parens;

    case u8')': return right_parens;

    case u8'{': return left_brace;

    case u8'}': return right_brace;

    case u8'[': return left_square;

    case u8']': return right_square;

    default: return {};
    }
}

} // namespace lua

bool highlight_lua(
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    std::pmr::memory_resource*,
    const Highlight_Options& options
)
{
    const auto emit = [&](std::size_t begin, std::size_t length, Highlight_Type type) {
        const bool coalesce = options.coalescing //
            && !out.empty() //
            && Highlight_Type(out.back().type) == type //
            && out.back().begin + out.back().length == begin;
        if (coalesce) {
            out.back().length += length;
        }
        else {
            out.emplace_back(begin, length, Underlying(type));
        }
    };

    std::size_t index = 0;

    while (index < source.size()) {
        const std::u8string_view remainder = source.substr(index);

        // Special case (s).
        if (remainder.length() >= 7 && remainder.substr(0, 7) == u8"<const>") {
            // Emit '<' with attr_delim highlight.
            emit(index, 1, Highlight_Type::attr_delim);
            // Emit 'const' with attr highlight.
            emit(index + 1, 5, Highlight_Type::attr);
            // Emit '>' with attr_delim highlight.
            emit(index + 6, 1, Highlight_Type::attr_delim);

            index += 7;
            continue;
        }

        // Whitespace.
        if (const std::size_t white_length = lua::match_whitespace(remainder)) {
            index += white_length;
            continue;
        }

        // Line comments - Lua treats each line separately, no backslash continuation
        if (const std::size_t line_comment_length = lua::match_line_comment(remainder)) {
            emit(index, 2, Highlight_Type::comment_delim);
            emit(index + 2, line_comment_length - 2, Highlight_Type::comment);
            index += line_comment_length;
            continue;
        }

        // Block comments.
        if (const lua::Comment_Result block_comment = lua::match_block_comment(remainder)) {
            // Prefix --[[ or --[=[.
            std::size_t prefix_length = 4; // Default for --[[.
            if (remainder[2] == u8'[' && remainder[3] != u8'[') {
                prefix_length = 3;
                while (prefix_length < remainder.length() && remainder[prefix_length] == u8'=') {
                    prefix_length++;
                }
                prefix_length++; // Handles the [ after equals.
            }

            const std::size_t closing_delim_length = block_comment.is_terminated
                ? (remainder[2] == u8'[' && remainder[3] != u8'[' ? 2 + (prefix_length - 4) : 2)
                : 0;

            emit(index, prefix_length, Highlight_Type::comment_delim);
            emit(
                index + prefix_length, block_comment.length - prefix_length - closing_delim_length,
                Highlight_Type::comment
            );

            if (block_comment.is_terminated) {
                emit(
                    index + block_comment.length - closing_delim_length, closing_delim_length,
                    Highlight_Type::comment_delim
                );
            }

            index += block_comment.length;
            continue;
        }

        // String literals.
        if (const lua::String_Literal_Result string = lua::match_string_literal(remainder)) {
            if (string.is_long_string) {
                // [[ ]] or [=[ ]=] multi-line strings, highlight the delimiters separately.
                std::size_t opening_delim_len = 2;
                std::size_t closing_delim_len = string.terminated ? 2 : 0;

                // [=[ style with equals signs.
                if (remainder[0] == u8'[' && remainder.length() >= 3 && remainder[1] == u8'=') {
                    opening_delim_len = 1;
                    while (opening_delim_len < remainder.length()
                           && remainder[opening_delim_len] == u8'=') {
                        opening_delim_len++;
                    }
                    opening_delim_len++; // Second [.

                    if (string.terminated) {
                        closing_delim_len = opening_delim_len;
                    }
                }

                emit(index, opening_delim_len, Highlight_Type::string);
                emit(
                    index + opening_delim_len,
                    string.length - opening_delim_len - closing_delim_len, Highlight_Type::string
                );

                if (string.terminated) {
                    emit(
                        index + string.length - closing_delim_len, closing_delim_len,
                        Highlight_Type::string
                    );
                }
            }
            else {
                // Regular quoted strings.
                emit(index, string.length, Highlight_Type::string);
            }

            index += string.length;
            continue;
        }

        // Number literals.
        if (const std::size_t number_length = lua::match_number(remainder)) {
            emit(index, number_length, Highlight_Type::number);
            index += number_length;
            continue;
        }

        // Identifiers and keywords.
        if (const std::size_t id_length = lua::match_identifier(remainder)) {
            const std::optional<lua::Lua_Token_Type> keyword
                = lua::lua_token_type_by_code(remainder.substr(0, id_length));

            const auto highlight
                = keyword ? lua::lua_token_type_highlight(*keyword) : Highlight_Type::id;

            emit(index, id_length, highlight);
            index += id_length;
            continue;
        }

        // Operation and punctuation.
        if (const std::optional<lua::Lua_Token_Type> op
            = lua::match_operator_or_punctuation(remainder)) {

            const std::size_t op_length = lua::lua_token_type_length(*op);
            const Highlight_Type op_highlight = lua::lua_token_type_highlight(*op);

            emit(index, op_length, op_highlight);
            index += op_length;
            continue;
        }

        emit(index, 1, Highlight_Type::sym);
        index++;
    }

    return true;
}

} // namespace ulight
