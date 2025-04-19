#ifndef ULIGHT_JS_HPP
#define ULIGHT_JS_HPP

#pragma once

#include <cstddef>
#include <optional>
#include <string_view>

#include "ulight/ulight.hpp"

namespace ulight::js {

enum struct Feature_Source : Underlying {
    /// @brief Common JavaScript
    js,
    /// @brief JSX features
    jsx,
    /// @brief Common between JavaScript and JSX
    js_jsx,
};

#define ULIGHT_JS_TOKEN_ENUM_DATA(F)                                                               \
    F(logical_not, "!", sym_op, js)                                                                \
    F(not_equals, "!=", sym_op, js)                                                                \
    F(strict_not_equals, "!==", sym_op, js)                                                        \
    F(modulo, "%", sym_op, js)                                                                     \
    F(modulo_equal, "%=", sym_op, js)                                                              \
    F(bitwise_and, "&", sym_op, js)                                                                \
    F(logical_and, "&&", sym_op, js)                                                               \
    F(logical_and_equal, "&&=", sym_op, js)                                                        \
    F(bitwise_and_equal, "&=", sym_op, js)                                                         \
    F(left_paren, "(", sym_parens, js_jsx)                                                         \
    F(right_paren, ")", sym_parens, js_jsx)                                                        \
    F(multiply, "*", sym_op, js)                                                                   \
    F(exponentiation, "**", sym_op, js)                                                            \
    F(exponentiation_equal, "**=", sym_op, js)                                                     \
    F(multiply_equal, "*=", sym_op, js)                                                            \
    F(plus, "+", sym_op, js)                                                                       \
    F(increment, "++", sym_op, js)                                                                 \
    F(plus_equal, "+=", sym_op, js)                                                                \
    F(comma, ",", sym_punc, js_jsx)                                                                \
    F(minus, "-", sym_op, js)                                                                      \
    F(decrement, "--", sym_op, js)                                                                 \
    F(minus_equal, "-=", sym_op, js)                                                               \
    F(dot, ".", sym_op, js_jsx)                                                                    \
    F(ellipsis, "...", sym_op, js_jsx)                                                             \
    F(divide, "/", sym_op, js)                                                                     \
    F(divide_equal, "/=", sym_op, js)                                                              \
    F(colon, ":", sym_op, js_jsx)                                                                  \
    F(semicolon, ";", sym_punc, js_jsx)                                                            \
    F(less_than, "<", sym_op, js_jsx)                                                              \
    F(left_shift, "<<", sym_op, js)                                                                \
    F(left_shift_equal, "<<=", sym_op, js)                                                         \
    F(less_equal, "<=", sym_op, js)                                                                \
    F(assignment, "=", sym_op, js)                                                                 \
    F(equals, "==", sym_op, js)                                                                    \
    F(strict_equals, "===", sym_op, js)                                                            \
    F(arrow, "=>", sym_op, js)                                                                     \
    F(greater_than, ">", sym_op, js)                                                               \
    F(greater_equal, ">=", sym_op, js)                                                             \
    F(right_shift, ">>", sym_op, js)                                                               \
    F(right_shift_equal, ">>=", sym_op, js)                                                        \
    F(unsigned_right_shift, ">>>", sym_op, js)                                                     \
    F(unsigned_right_shift_equal, ">>>=", sym_op, js)                                              \
    F(conditional, "?", sym_op, js)                                                                \
    F(optional_chaining, "?.", sym_op, js)                                                         \
    F(nullish_coalescing, "??", sym_op, js)                                                        \
    F(nullish_coalescing_equal, "??=", sym_op, js)                                                 \
    F(left_bracket, "[", sym_square, js_jsx)                                                       \
    F(right_bracket, "]", sym_square, js_jsx)                                                      \
    F(bitwise_xor, "^", sym_op, js)                                                                \
    F(bitwise_xor_equal, "^=", sym_op, js)                                                         \
    F(kw_as, "as", keyword, js)                                                                    \
    F(kw_async, "async", keyword, js)                                                              \
    F(kw_await, "await", keyword, js)                                                              \
    F(kw_break, "break", keyword_control, js)                                                      \
    F(kw_case, "case", keyword_control, js)                                                        \
    F(kw_catch, "catch", keyword_control, js)                                                      \
    F(kw_class, "class", keyword, js)                                                              \
    F(kw_const, "const", keyword, js)                                                              \
    F(kw_continue, "continue", keyword_control, js)                                                \
    F(kw_debugger, "debugger", keyword, js)                                                        \
    F(kw_default, "default", keyword_control, js)                                                  \
    F(kw_delete, "delete", keyword, js)                                                            \
    F(kw_do, "do", keyword_control, js)                                                            \
    F(kw_else, "else", keyword_control, js)                                                        \
    F(kw_enum, "enum", keyword, js)                                                                \
    F(kw_export, "export", keyword, js)                                                            \
    F(kw_extends, "extends", keyword, js)                                                          \
    F(kw_false, "false", bool_, js)                                                                \
    F(kw_finally, "finally", keyword_control, js)                                                  \
    F(kw_for, "for", keyword_control, js)                                                          \
    F(kw_from, "from", keyword, js)                                                                \
    F(kw_function, "function", keyword, js)                                                        \
    F(kw_get, "get", keyword, js)                                                                  \
    F(kw_if, "if", keyword_control, js)                                                            \
    F(kw_import, "import", keyword, js)                                                            \
    F(kw_in, "in", keyword, js)                                                                    \
    F(kw_instanceof, "instanceof", keyword, js)                                                    \
    F(kw_let, "let", keyword, js)                                                                  \
    F(kw_new, "new", keyword, js)                                                                  \
    F(kw_null, "null", null, js)                                                                   \
    F(kw_of, "of", keyword, js)                                                                    \
    F(kw_return, "return", keyword_control, js)                                                    \
    F(kw_set, "set", keyword, js)                                                                  \
    F(kw_static, "static", keyword, js)                                                            \
    F(kw_super, "super", keyword, js)                                                              \
    F(kw_switch, "switch", keyword_control, js)                                                    \
    F(kw_this, "this", this_, js)                                                                  \
    F(kw_throw, "throw", keyword_control, js)                                                      \
    F(kw_true, "true", bool_, js)                                                                  \
    F(kw_try, "try", keyword_control, js)                                                          \
    F(kw_typeof, "typeof", keyword, js)                                                            \
    F(kw_var, "var", keyword, js)                                                                  \
    F(kw_void, "void", keyword, js)                                                                \
    F(kw_while, "while", keyword_control, js)                                                      \
    F(kw_with, "with", keyword_control, js)                                                        \
    F(kw_yield, "yield", keyword, js)                                                              \
    F(left_brace, "{", sym_brace, js_jsx)                                                          \
    F(bitwise_or, "|", sym_op, js)                                                                 \
    F(bitwise_or_equal, "|=", sym_op, js)                                                          \
    F(logical_or, "||", sym_op, js)                                                                \
    F(logical_or_equal, "||=", sym_op, js)                                                         \
    F(right_brace, "}", sym_brace, js_jsx)                                                         \
    F(bitwise_not, "~", sym_op, js)

#define ULIGHT_JS_TOKEN_ENUM_ENUMERATOR(id, code, highlight, source) id,

enum struct Token_Type : Underlying { //
    ULIGHT_JS_TOKEN_ENUM_DATA(ULIGHT_JS_TOKEN_ENUM_ENUMERATOR)
};

inline constexpr auto js_token_type_count = std::size_t(Token_Type::bitwise_not) + 1;

/// @brief Returns the in-code representation of `type`.
/// For example, if `type` is `plus`, returns `"+"`.
/// If `type` is invalid, returns an empty string.
[[nodiscard]]
std::u8string_view js_token_type_code(Token_Type type) noexcept;

/// @brief Equivalent to `js_token_type_code(type).length()`.
[[nodiscard]]
std::size_t js_token_type_length(Token_Type type) noexcept;

[[nodiscard]]
Highlight_Type js_token_type_highlight(Token_Type type) noexcept;

[[nodiscard]]
Feature_Source js_token_type_source(Token_Type type) noexcept;

[[nodiscard]]
std::optional<Token_Type> js_token_type_by_code(std::u8string_view code) noexcept;

[[nodiscard]]
bool starts_with_line_terminator(std::u8string_view s);

[[nodiscard]]
std::size_t match_line_terminator_sequence(std::u8string_view s);

/// @brief Matches zero or more characters for which `is_js_whitespace` is `true`.
[[nodiscard]]
std::size_t match_whitespace(std::u8string_view str);

/// @brief Matches zero or more characters for which `is_js_whitespace` is `false`.
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

/// @brief Returns a match for a JavaScript block comment at the start of `str`, if any.
/// JavaScript block comments start with /* and end with */
[[nodiscard]]
Comment_Result match_block_comment(std::u8string_view str) noexcept;

/// @brief Returns the length of a JavaScript line comment at the start of `str`, if any.
/// Returns zero if there is no line comment.
/// In any case, the length does not include the terminating newline character.
/// JavaScript line comments start with //
[[nodiscard]]
std::size_t match_line_comment(std::u8string_view str) noexcept;

/// @brief Returns the length of a JavaScript hashbang comment at the start of `str`, if any.
/// Returns zero if there is no hashbang comment.
/// The hashbang comment must appear at the very beginning of the file to be valid.
/// JavaScript hashbang comments start with #! and are only valid as the first line
[[nodiscard]]
std::size_t match_hashbang_comment(std::u8string_view str) noexcept;

struct String_Literal_Result {
    std::size_t length;
    bool terminated;

    [[nodiscard]]
    constexpr explicit operator bool() const noexcept
    {
        return length != 0;
    }

    [[nodiscard]]
    friend constexpr bool operator==(String_Literal_Result, String_Literal_Result)
        = default;
};

/// @brief Matches a JavaScript string literal at the start of `str`.
[[nodiscard]]
String_Literal_Result match_string_literal(std::u8string_view str);

#if 0
/// @brief Matches a JavaScript template literal at the start of `str`.
[[nodiscard]]
String_Literal_Result match_template(std::u8string_view str);
#endif

/// @brief Matches a JavaScript template literal substitution at the start of `str`.
/// Returns the position after the closing } if found.
[[nodiscard]]
std::size_t match_template_substitution(std::u8string_view str);

struct Digits_Result {
    std::size_t length;
    /// @brief If `true`, does not satisfy the rules for a digit sequence.
    /// In particular, digit separators cannot be leading or trailing,
    /// and there cannot be multiple consecutive digit separators.
    bool erroneous;

    [[nodiscard]]
    constexpr explicit operator bool() const noexcept
    {
        return length != 0;
    }

    [[nodiscard]]
    friend constexpr bool operator==(Digits_Result, Digits_Result)
        = default;
};

/// @brief Matches a JavaScript sequence of digits,
/// possibly containing `_` separators, in the given base.
/// @param base The base of integer, in range [2, 16].
[[nodiscard]]
Digits_Result match_digits(std::u8string_view str, int base = 10);

struct Numeric_Result {
    /// @brief The total length.
    /// This is also the sum of all the other parts of the result.
    std::size_t length = 0;
    /// @brief The length of the prefix (e.g. `0x`) or zero if none present.
    std::size_t prefix = 0;
    /// @brief The length of the digits prior to the radix point or exponent part.
    std::size_t integer = 0;
    /// @brief The length of the fractional part, including the radix point.
    std::size_t fractional = 0;
    /// @brief The length of the exponent part, including the `E+`, `e-`, etc. or other prefix.
    std::size_t exponent = 0;
    /// @brief The length of the suffix (`n`) or zero if none present.
    std::size_t suffix = 0;
    /// @brief If `true`, was recognized as a number,
    /// but does not satisfy some rule related to numeric literals.
    bool erroneous = false;

    [[nodiscard]]
    constexpr explicit operator bool() const
    {
        return length != 0;
    }

    [[nodiscard]]
    friend constexpr bool operator==(Numeric_Result, Numeric_Result)
        = default;
};

/// @brief Matches a JavaScript numeric literal at the start of `str`.
/// Handles integers, decimals, hex, binary, octal, BigInt, scientific notation, and separators
[[nodiscard]]
Numeric_Result match_numeric_literal(std::u8string_view str);

/// @brief Matches a JavaScript identifier at the start of `str`
/// and returns its length.
/// If none could be matched, returns zero.
[[nodiscard]]
std::size_t match_identifier(std::u8string_view str);

/// @brief Like `match_identifier`, but also accepts identifiers that contain `-`
/// anywhere but the first character.
[[nodiscard]]
std::size_t match_jsx_identifier(std::u8string_view str);

/// @brief Matches a JavaScript private identifier (starting with #) at the start of `str`
/// and returns its length.
/// If none could be matched, returns zero.
[[nodiscard]]
std::size_t match_private_identifier(std::u8string_view str);

/// @brief Matches a JavaScript operator or punctuation at the start of `str`.
[[nodiscard]]
std::optional<Token_Type> match_operator_or_punctuation(std::u8string_view str, bool js_or_jsx);

enum struct JSX_Type : Underlying {
    /// @brief JSXOpeningElement, e.g. `<div>`.
    opening,
    /// @brief JSXClosingElement, e.g. `</div>`.
    closing,
    /// @brief JSXSelfCLosingElement, e.g. `<br/>`.
    self_closing,
    /// @brief Start of JSXFragment, e.g. `<>`.
    fragment_opening,
    /// @brief End of JSXFragment, e.g. `</>`.
    fragment_closing,
};

[[nodiscard]]
constexpr std::size_t jsx_type_prefix_length(JSX_Type type)
{
    return type == JSX_Type::closing || type == JSX_Type::fragment_closing ? 2 : 1;
}

[[nodiscard]]
constexpr std::size_t jsx_type_suffix_length(JSX_Type type)
{
    return type == JSX_Type::self_closing ? 2 : 1;
}

[[nodiscard]]
constexpr bool jsx_type_is_closing(JSX_Type type)
{
    return type == JSX_Type::closing || type == JSX_Type::fragment_closing;
}

struct JSX_Tag_Result {
    std::size_t length;
    JSX_Type type;

    [[nodiscard]]
    constexpr explicit operator bool() const
    {
        return length != 0;
    }

    [[nodiscard]]
    friend constexpr bool operator==(JSX_Tag_Result, JSX_Tag_Result)
        = default;
};

/// @brief Matches a some kind of JSX element or fragment tag at the start of `str`.
/// Note that because JSX elements are specified on top of the non-lexical grammar,
/// comments and whitespace can be added anywhere with no effect,
/// leading to more permissive syntax than HTML.
///
/// For example, `<br /* comment *//>` is a valid self-closing element.
[[nodiscard]]
JSX_Tag_Result match_jsx_tag(std::u8string_view str);

using JSX_Braced_Result = Comment_Result;

[[nodiscard]]
JSX_Braced_Result match_jsx_braced(std::u8string_view str);

/// @brief Matches a JSX tag name at the start of the string
[[nodiscard]]
std::size_t match_jsx_element_name(std::u8string_view str);

/// @brief Matches a JSX attribute name at the start of the string
[[nodiscard]]
std::size_t match_jsx_attribute_name(std::u8string_view str);

} // namespace ulight::js

#endif
