#ifndef ULIGHT_BASH_HPP
#define ULIGHT_BASH_HPP

#include <algorithm>
#include <optional>
#include <string_view>

#include "ulight/ulight.hpp"

#include "ulight/impl/platform.h"

namespace ulight::bash {

#define ULIGHT_BASH_TOKEN_ENUM_DATA(F)                                                             \
    F(exclamation, "!", sym_op)                                                                    \
    F(dollar, "$", sym)                                                                            \
    F(dollar_quote, "$'", sym_parens)                                                              \
    F(dollar_parens, "$(", sym_parens)                                                             \
    F(dollar_brace, "${", sym_brace)                                                               \
    F(amp, "&", sym_op)                                                                            \
    F(amp_amp, "&&", sym_op)                                                                       \
    F(amp_greater, "&>", sym_op)                                                                   \
    F(amp_greater_greater, "&>>", sym_op)                                                          \
    F(left_parens, "(", sym_parens)                                                                \
    F(right_parens, ")", sym_parens)                                                               \
    F(asterisk, "*", sym_op)                                                                       \
    F(plus, "+", sym_op)                                                                           \
    F(minus, "-", sym_op)                                                                          \
    F(colon, ":", sym_punc)                                                                        \
    F(semicolon, ";", sym_punc)                                                                    \
    F(less, "<", sym_op)                                                                           \
    F(less_amp, "<&", sym_op)                                                                      \
    F(less_less, "<<", sym_op)                                                                     \
    F(less_less_less, "<<<", sym_op)                                                               \
    F(less_greater, "<>", sym_op)                                                                  \
    F(equal, "=", sym_op)                                                                          \
    F(greater, ">", sym_op)                                                                        \
    F(greater_amp, ">&", sym_op)                                                                   \
    F(greater_greater, ">>", sym_op)                                                               \
    F(question, "?", sym_op)                                                                       \
    F(at, "@", sym_op)                                                                             \
    F(left_square, "[", sym_square)                                                                \
    F(left_square_square, "[[", sym_square)                                                        \
    F(right_square, "]", sym_square)                                                               \
    F(right_square_square, "]]", sym_square)                                                       \
    F(kw_case, "case", keyword_control)                                                            \
    F(kw_coproc, "coproc", keyword_control)                                                        \
    F(kw_do, "do", keyword_control)                                                                \
    F(kw_done, "done", keyword_control)                                                            \
    F(kw_elif, "elif", keyword_control)                                                            \
    F(kw_else, "else", keyword_control)                                                            \
    F(kw_esac, "esac", keyword_control)                                                            \
    F(kw_fi, "fi", keyword_control)                                                                \
    F(kw_for, "for", keyword_control)                                                              \
    F(kw_function, "function", keyword)                                                            \
    F(kw_if, "if", keyword_control)                                                                \
    F(kw_in, "in", keyword)                                                                        \
    F(kw_select, "select", keyword)                                                                \
    F(kw_then, "then", keyword_control)                                                            \
    F(kw_time, "time", keyword)                                                                    \
    F(kw_until, "until", keyword_control)                                                          \
    F(kw_while, "while", keyword_control)                                                          \
    F(left_brace, "{", sym_brace)                                                                  \
    F(pipe, "|", sym_op)                                                                           \
    F(pipe_pipe, "||", sym_op)                                                                     \
    F(right_brace, "}", sym_brace)                                                                 \
    F(tilde, "~", sym_op)

#define ULIGHT_BASH_TOKEN_ENUMERATOR(id, code, highlight) id,
#define ULIGHT_BASH_TOKEN_CODE8(id, code, highlight) u8##code,
#define ULIGHT_BASH_TOKEN_LENGTH(id, code, highlight) (sizeof(u8##code) - 1),
#define ULIGHT_BASH_TOKEN_HIGHLIGHT_TYPE(id, code, highlight) Highlight_Type::highlight,

enum struct Token_Type : Underlying {
    ULIGHT_BASH_TOKEN_ENUM_DATA(ULIGHT_BASH_TOKEN_ENUMERATOR)
};

[[nodiscard]]
std::optional<Token_Type> match_operator(std::u8string_view str);

struct String_Result {
    std::size_t length;
    bool terminated;

    [[nodiscard]]
    constexpr explicit operator bool() const
    {
        return length != 0;
    }

    [[nodiscard]]
    friend constexpr bool operator==(String_Result, String_Result)
        = default;
};

[[nodiscard]]
String_Result match_single_quoted_string(std::u8string_view str);

[[nodiscard]]
std::size_t match_comment(std::u8string_view str);

[[nodiscard]]
std::size_t match_blank(std::u8string_view str);

[[nodiscard]]
std::size_t match_word(std::u8string_view str);

[[nodiscard]]
bool starts_with_substitution(std::u8string_view str);

[[nodiscard]]
std::size_t match_identifier(std::u8string_view str);

} // namespace ulight::bash

#endif
