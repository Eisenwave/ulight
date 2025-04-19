#ifndef ULIGHT_LUA_HPP
#define ULIGHT_LUA_HPP

#pragma once

#include <cstddef>
#include <optional>
#include <string_view>

#include "ulight/ulight.hpp"

namespace ulight::lua {

#define ULIGHT_LUA_TOKEN_ENUM_DATA(F)                                                              \
    F(hash, "#", sym_op, 1)                                                                        \
    F(percent, "%", sym_op, 1)                                                                     \
    F(amp, "&", sym_op, 1)                                                                         \
    F(left_parens, "(", sym_parens, 1)                                                             \
    F(right_parens, ")", sym_parens, 1)                                                            \
    F(asterisk, "*", sym_op, 1)                                                                    \
    F(plus, "+", sym_op, 1)                                                                        \
    F(comma, ",", sym_punc, 1)                                                                     \
    F(minus, "-", sym_op, 1)                                                                       \
    F(dot, ".", sym_op, 1)                                                                         \
    F(dot_dot, "..", sym_op, 1)                                                                    \
    F(dot_dot_dot, "...", sym_op, 1)                                                               \
    F(slash, "/", sym_op, 1)                                                                       \
    F(floor_div, "//", sym_op, 1)                                                                  \
    F(colon, ":", sym_op, 1)                                                                       \
    F(colon_colon, "::", sym_op, 1)                                                                \
    F(semicolon, ";", sym_punc, 1)                                                                 \
    F(less, "<", sym_op, 1)                                                                        \
    F(left_shift, "<<", sym_op, 1)                                                                 \
    F(less_eq, "<=", sym_op, 1)                                                                    \
    F(eq, "=", sym_op, 1)                                                                          \
    F(eq_eq, "==", sym_op, 1)                                                                      \
    F(greater, ">", sym_op, 1)                                                                     \
    F(greater_eq, ">=", sym_op, 1)                                                                 \
    F(right_shift, ">>", sym_op, 1)                                                                \
    F(left_square, "[", sym_square, 1)                                                             \
    F(right_square, "]", sym_square, 1)                                                            \
    F(caret, "^", sym_op, 1)                                                                       \
    F(kw_and, "and", keyword, 1)                                                                   \
    F(kw_break, "break", keyword_control, 1)                                                       \
    F(kw_do, "do", keyword_control, 1)                                                             \
    F(kw_else, "else", keyword_control, 1)                                                         \
    F(kw_elseif, "elseif", keyword_control, 1)                                                     \
    F(kw_end, "end", keyword_control, 1)                                                           \
    F(kw_false, "false", bool_, 1)                                                                 \
    F(kw_for, "for", keyword_control, 1)                                                           \
    F(kw_function, "function", keyword, 1)                                                         \
    F(kw_goto, "goto", keyword_control, 1)                                                         \
    F(kw_if, "if", keyword_control, 1)                                                             \
    F(kw_in, "in", keyword, 1)                                                                     \
    F(kw_local, "local", keyword, 1)                                                               \
    F(kw_nil, "nil", null, 1)                                                                      \
    F(kw_not, "not", keyword, 1)                                                                   \
    F(kw_or, "or", keyword, 1)                                                                     \
    F(kw_repeat, "repeat", keyword_control, 1)                                                     \
    F(kw_return, "return", keyword_control, 1)                                                     \
    F(kw_then, "then", keyword_control, 1)                                                         \
    F(kw_true, "true", bool_, 1)                                                                   \
    F(kw_until, "until", keyword_control, 1)                                                       \
    F(kw_while, "while", keyword_control, 1)                                                       \
    F(left_brace, "{", sym_brace, 1)                                                               \
    F(pipe, "|", sym_op, 1)                                                                        \
    F(right_brace, "}", sym_brace, 1)                                                              \
    F(tilde, "~", sym_op, 1)                                                                       \
    F(tilde_eq, "~=", sym_op, 1)

#define ULIGHT_LUA_TOKEN_ENUM_ENUMERATOR(id, code, highlight, strict) id,

enum struct Lua_Token_Type : Underlying { //
    ULIGHT_LUA_TOKEN_ENUM_DATA(ULIGHT_LUA_TOKEN_ENUM_ENUMERATOR)
};

inline constexpr auto lua_token_type_count = std::size_t(Lua_Token_Type::tilde_eq) + 1;

/// @brief Returns the in-code representation of `type`.
/// For example, if `type` is `plus`, returns `"+"`.
/// If `type` is invalid, returns an empty string.
[[nodiscard]]
std::u8string_view lua_token_type_code(Lua_Token_Type type) noexcept;

/// @brief Equivalent to `lua_token_type_code(type).length()`.
[[nodiscard]]
std::size_t lua_token_type_length(Lua_Token_Type type) noexcept;

[[nodiscard]]
Highlight_Type lua_token_type_highlight(Lua_Token_Type type) noexcept;

[[nodiscard]]
bool lua_token_type_is_strict(Lua_Token_Type type) noexcept;

[[nodiscard]]
std::optional<Lua_Token_Type> lua_token_type_by_code(std::u8string_view code) noexcept;

/// @brief Matches zero or more characters for which `is_lua_whitespace` is `true`.
[[nodiscard]]
std::size_t match_whitespace(std::u8string_view str);

/// @brief Matches zero or more characters for which `is_lua_whitespace` is `false`.
[[nodiscard]]
std::size_t match_non_whitespace(std::u8string_view str);

struct Comment_Result {
    std::size_t length;
    bool is_terminated;

    [[nodiscard]]
    constexpr explicit operator bool() const noexcept
    {
        return length != 0;
    }
};

/// @brief Returns a match for a Lua block comment at the start of `str`, if any.
/// Lua block comments start with --[[ and end with ]]
[[nodiscard]]
Comment_Result match_block_comment(std::u8string_view str) noexcept;

/// @brief Returns the length of a Lua line comment at the start of `str`, if any.
/// Returns zero if there is no line comment.
/// In any case, the length does not include the terminating newline character.
/// Lua line comments start with --
[[nodiscard]]
std::size_t match_line_comment(std::u8string_view str) noexcept;

struct String_Literal_Result {
    std::size_t length;
    bool is_long_string; // For [[ ]] style strings
    bool terminated;

    [[nodiscard]]
    constexpr explicit operator bool() const noexcept
    {
        return length != 0;
    }
};

/// @brief Matches a Lua string literal at the start of `str`.
/// Handles both quoted strings ("" or '') and long bracket strings ([[ ]] or [=[ ]=])
[[nodiscard]]
String_Literal_Result match_string_literal(std::u8string_view str);

/// @brief Matches a Lua number literal at the start of `str`.
/// Handles integers, decimals, hex, and scientific notation
[[nodiscard]]
std::size_t match_number(std::u8string_view str);

/// @brief Matches a Lua identifier at the start of `str`
/// and returns its length.
/// If none could be matched, returns zero.
[[nodiscard]]
std::size_t match_identifier(std::u8string_view str);

/// @brief Matches a Lua operator or punctuation at the start of `str`.
[[nodiscard]]
std::optional<Lua_Token_Type> match_operator_or_punctuation(std::u8string_view str);

} // namespace ulight::lua

#endif
