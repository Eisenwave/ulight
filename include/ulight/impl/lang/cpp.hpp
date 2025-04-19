#ifndef ULIGHT_CPP_HPP
#define ULIGHT_CPP_HPP

#include <cstddef>
#include <optional>
#include <string_view>

#include "ulight/ulight.hpp"

namespace ulight::cpp {

enum struct Feature_Source : Underlying {
    /// @brief Compiler extensions. Neither standard C nor standard C++.
    ext,
    /// @brief Standard C features.
    c,
    /// @brief Standard C++ features.
    cpp,
    /// @brief Common C and C++ features.
    c_cpp,
};

[[nodiscard]]
constexpr bool is_c_feature(Feature_Source source)
{
    return source == Feature_Source::c || source == Feature_Source::c_cpp;
}

[[nodiscard]]
constexpr bool is_cpp_feature(Feature_Source source)
{
    return source == Feature_Source::cpp || source == Feature_Source::c_cpp;
}

#define ULIGHT_CPP_TOKEN_ENUM_DATA(F)                                                              \
    F(exclamation, "!", sym_op, c_cpp)                                                             \
    F(exclamation_eq, "!=", sym_op, c_cpp)                                                         \
    F(pound, "#", macro, c_cpp)                                                                    \
    F(pound_pound, "##", macro, c_cpp)                                                             \
    F(percent, "%", sym_op, c_cpp)                                                                 \
    F(pound_alt, "%:", macro, c_cpp)                                                               \
    F(pound_pound_alt, "%:%:", macro, c_cpp)                                                       \
    F(percent_eq, "%=", sym_op, c_cpp)                                                             \
    F(right_brace_alt, "%>", sym_brace, c_cpp)                                                     \
    F(amp, "&", sym_op, c_cpp)                                                                     \
    F(amp_amp, "&&", sym_op, c_cpp)                                                                \
    F(amp_eq, "&=", sym_op, c_cpp)                                                                 \
    F(left_parens, "(", sym_parens, c_cpp)                                                         \
    F(right_parens, ")", sym_parens, c_cpp)                                                        \
    F(asterisk, "*", sym_op, c_cpp)                                                                \
    F(asterisk_eq, "*=", sym_op, c_cpp)                                                            \
    F(plus, "+", sym_op, c_cpp)                                                                    \
    F(plus_plus, "++", sym_op, c_cpp)                                                              \
    F(plus_eq, "+=", sym_op, c_cpp)                                                                \
    F(comma, ",", sym_punc, c_cpp)                                                                 \
    F(minus, "-", sym_op, c_cpp)                                                                   \
    F(minus_minus, "--", sym_op, c_cpp)                                                            \
    F(minus_eq, "-=", sym_op, c_cpp)                                                               \
    F(arrow, "->", sym_op, c_cpp)                                                                  \
    F(member_arrow_access, "->*", sym_op, cpp)                                                     \
    F(dot, ".", sym_op, c_cpp)                                                                     \
    F(member_pointer_access, ".*", sym_op, cpp)                                                    \
    F(ellipsis, "...", sym_op, c_cpp)                                                              \
    F(slash, "/", sym_op, c_cpp)                                                                   \
    F(slash_eq, "/=", sym_op, c_cpp)                                                               \
    F(colon, ":", sym_punc, c_cpp)                                                                 \
    F(scope, "::", sym_op, cpp)                                                                    \
    F(right_square_alt, ":>", sym_square, c_cpp)                                                   \
    F(semicolon, ";", sym_punc, c_cpp)                                                             \
    F(less, "<", sym_op, c_cpp)                                                                    \
    F(left_brace_alt, "<%", sym_brace, c_cpp)                                                      \
    F(left_square_alt, "<:", sym_square, c_cpp)                                                    \
    F(less_less, "<<", sym_op, c_cpp)                                                              \
    F(less_less_eq, "<<=", sym_op, c_cpp)                                                          \
    F(less_eq, "<=", sym_op, c_cpp)                                                                \
    F(three_way, "<=>", sym_op, cpp)                                                               \
    F(eq, "=", sym_op, c_cpp)                                                                      \
    F(eq_eq, "==", sym_op, c_cpp)                                                                  \
    F(greater, ">", sym_op, c_cpp)                                                                 \
    F(greater_eq, ">=", sym_op, c_cpp)                                                             \
    F(greater_greater, ">>", sym_op, c_cpp)                                                        \
    F(greater_greater_eq, ">>=", sym_op, c_cpp)                                                    \
    F(question, "?", sym_op, c_cpp)                                                                \
    F(left_square, "[", sym_square, c_cpp)                                                         \
    F(right_square, "]", sym_square, c_cpp)                                                        \
    F(caret, "^", sym_op, c_cpp)                                                                   \
    F(caret_eq, "^=", sym_op, c_cpp)                                                               \
    F(caret_caret, "^^", sym_op, cpp)                                                              \
    F(c_alignas, "_Alignas", keyword, c)                                                           \
    F(c_alignof, "_Alignof", keyword, c)                                                           \
    F(c_atomic, "_Atomic", keyword, c_cpp)                                                         \
    F(c_bitint, "_BitInt", keyword_type, c)                                                        \
    F(c_bool, "_Bool", keyword_type, c)                                                            \
    F(c_complex, "_Complex", keyword, c)                                                           \
    F(c_decimal128, "_Decimal128", keyword_type, c)                                                \
    F(c_decimal32, "_Decimal32", keyword_type, c)                                                  \
    F(c_decimal64, "_Decimal64", keyword_type, c)                                                  \
    F(c_float128, "_Float128", keyword_type, c)                                                    \
    F(c_float128x, "_Float128x", keyword_type, c)                                                  \
    F(c_float16, "_Float16", keyword_type, c)                                                      \
    F(c_float32, "_Float32", keyword_type, c)                                                      \
    F(c_float32x, "_Float32x", keyword_type, c)                                                    \
    F(c_float64, "_Float64", keyword_type, c)                                                      \
    F(c_float64x, "_Float64x", keyword_type, c)                                                    \
    F(c_generic, "_Generic", keyword, c)                                                           \
    F(c_imaginary, "_Imaginary", keyword, c)                                                       \
    F(c_noreturn, "_Noreturn", keyword, c)                                                         \
    F(c_pragma, "_Pragma", keyword, c_cpp)                                                         \
    F(c_static_assert, "_Static_assert", keyword, c)                                               \
    F(c_thread_local, "_Thread_local", keyword, c)                                                 \
    F(gnu_asm, "__asm__", keyword, ext)                                                            \
    F(gnu_attribute, "__attribute__", keyword, ext)                                                \
    F(gnu_extension, "__extension__", keyword, ext)                                                \
    F(gnu_float128, "__float128", keyword_type, ext)                                               \
    F(gnu_float80, "__float80", keyword_type, ext)                                                 \
    F(gnu_fp16, "__fp16", keyword_type, ext)                                                       \
    F(gnu_ibm128, "__ibm128", keyword_type, ext)                                                   \
    F(gnu_imag, "__imag__", keyword, ext)                                                          \
    F(ext_int128, "__int128", keyword_type, ext)                                                   \
    F(ext_int16, "__int16", keyword_type, ext)                                                     \
    F(ext_int256, "__int256", keyword_type, ext)                                                   \
    F(ext_int32, "__int32", keyword_type, ext)                                                     \
    F(ext_int64, "__int64", keyword_type, ext)                                                     \
    F(ext_int8, "__int8", keyword_type, ext)                                                       \
    F(gnu_label, "__label__", keyword, ext)                                                        \
    F(intel_m128, "__m128", keyword_type, ext)                                                     \
    F(intel_m128d, "__m128d", keyword_type, ext)                                                   \
    F(intel_m128i, "__m128i", keyword_type, ext)                                                   \
    F(intel_m256, "__m256", keyword_type, ext)                                                     \
    F(intel_m256d, "__m256d", keyword_type, ext)                                                   \
    F(intel_m256i, "__m256i", keyword_type, ext)                                                   \
    F(intel_m512, "__m512", keyword_type, ext)                                                     \
    F(intel_m512d, "__m512d", keyword_type, ext)                                                   \
    F(intel_m512i, "__m512i", keyword_type, ext)                                                   \
    F(intel_m64, "__m64", keyword_type, ext)                                                       \
    F(intel_mmask16, "__mmask16", keyword_type, ext)                                               \
    F(intel_mmask32, "__mmask32", keyword_type, ext)                                               \
    F(intel_mmask64, "__mmask64", keyword_type, ext)                                               \
    F(intel_mmask8, "__mmask8", keyword_type, ext)                                                 \
    F(microsoft_ptr32, "__ptr32", keyword_type, ext)                                               \
    F(microsoft_ptr64, "__ptr64", keyword_type, ext)                                               \
    F(gnu_real, "__real__", keyword, ext)                                                          \
    F(gnu_restrict, "__restrict", keyword, ext)                                                    \
    F(kw_alignas, "alignas", keyword, c_cpp)                                                       \
    F(kw_alignof, "alignof", keyword, c_cpp)                                                       \
    F(kw_and, "and", keyword, c_cpp)                                                               \
    F(kw_and_eq, "and_eq", keyword, c_cpp)                                                         \
    F(kw_asm, "asm", keyword_control, c_cpp)                                                       \
    F(kw_auto, "auto", keyword, c_cpp)                                                             \
    F(kw_bitand, "bitand", keyword, c_cpp)                                                         \
    F(kw_bitor, "bitor", keyword, c_cpp)                                                           \
    F(kw_bool, "bool", keyword_type, c_cpp)                                                        \
    F(kw_break, "break", keyword_control, c_cpp)                                                   \
    F(kw_case, "case", keyword_control, c_cpp)                                                     \
    F(kw_catch, "catch", keyword_control, c_cpp)                                                   \
    F(kw_char, "char", keyword_type, c_cpp)                                                        \
    F(kw_char16_t, "char16_t", keyword_type, cpp)                                                  \
    F(kw_char32_t, "char32_t", keyword_type, cpp)                                                  \
    F(kw_char8_t, "char8_t", keyword_type, cpp)                                                    \
    F(kw_class, "class", keyword, cpp)                                                             \
    F(kw_co_await, "co_await", keyword_control, cpp)                                               \
    F(kw_co_return, "co_return", keyword_control, cpp)                                             \
    F(kw_compl, "compl", keyword, c_cpp)                                                           \
    F(kw_complex, "complex", keyword, c)                                                           \
    F(kw_concept, "concept", keyword, cpp)                                                         \
    F(kw_const, "const", keyword, c_cpp)                                                           \
    F(kw_const_cast, "const_cast", keyword, cpp)                                                   \
    F(kw_consteval, "consteval", keyword, cpp)                                                     \
    F(kw_constexpr, "constexpr", keyword, c_cpp)                                                   \
    F(kw_constinit, "constinit", keyword, cpp)                                                     \
    F(kw_continue, "continue", keyword_control, c_cpp)                                             \
    F(kw_contract_assert, "contract_assert", keyword, cpp)                                         \
    F(kw_decltype, "decltype", keyword, cpp)                                                       \
    F(kw_default, "default", keyword, c_cpp)                                                       \
    F(kw_delete, "delete", keyword, cpp)                                                           \
    F(kw_do, "do", keyword_control, c_cpp)                                                         \
    F(kw_double, "double", keyword_type, c_cpp)                                                    \
    F(kw_dynamic_cast, "dynamic_cast", keyword, cpp)                                               \
    F(kw_else, "else", keyword_control, c_cpp)                                                     \
    F(kw_enum, "enum", keyword, c_cpp)                                                             \
    F(kw_explicit, "explicit", keyword, cpp)                                                       \
    F(kw_export, "export", keyword, cpp)                                                           \
    F(kw_extern, "extern", keyword, c_cpp)                                                         \
    F(kw_false, "false", bool_, c_cpp)                                                             \
    F(kw_final, "final", keyword, cpp)                                                             \
    F(kw_float, "float", keyword_type, c_cpp)                                                      \
    F(kw_for, "for", keyword_control, c_cpp)                                                       \
    F(kw_friend, "friend", keyword, cpp)                                                           \
    F(kw_goto, "goto", keyword_control, c_cpp)                                                     \
    F(kw_if, "if", keyword_control, c_cpp)                                                         \
    F(kw_imaginary, "imaginary", keyword, c)                                                       \
    F(kw_import, "import", keyword, cpp)                                                           \
    F(kw_inline, "inline", keyword, c_cpp)                                                         \
    F(kw_int, "int", keyword_type, c_cpp)                                                          \
    F(kw_long, "long", keyword_type, c_cpp)                                                        \
    F(kw_module, "module", keyword, cpp)                                                           \
    F(kw_mutable, "mutable", keyword, cpp)                                                         \
    F(kw_namespace, "namespace", keyword, cpp)                                                     \
    F(kw_new, "new", keyword, cpp)                                                                 \
    F(kw_noexcept, "noexcept", keyword, cpp)                                                       \
    F(kw_noreturn, "noreturn", keyword, c)                                                         \
    F(kw_not, "not", keyword, c_cpp)                                                               \
    F(kw_not_eq, "not_eq", keyword, c_cpp)                                                         \
    F(kw_nullptr, "nullptr", null, c_cpp)                                                          \
    F(kw_operator, "operator", keyword, cpp)                                                       \
    F(kw_or, "or", keyword, c_cpp)                                                                 \
    F(kw_or_eq, "or_eq", keyword, c_cpp)                                                           \
    F(kw_override, "override", keyword, cpp)                                                       \
    F(kw_post, "post", keyword, cpp)                                                               \
    F(kw_pre, "pre", keyword, cpp)                                                                 \
    F(kw_private, "private", keyword, cpp)                                                         \
    F(kw_protected, "protected", keyword, cpp)                                                     \
    F(kw_public, "public", keyword, cpp)                                                           \
    F(kw_register, "register", keyword, c_cpp)                                                     \
    F(kw_reinterpret_cast, "reinterpret_cast", keyword, cpp)                                       \
    F(kw_replaceable_if_eligible, "replaceable_if_eligible", keyword, cpp)                         \
    F(kw_requires, "requires", keyword, cpp)                                                       \
    F(kw_restrict, "restrict", keyword, c)                                                         \
    F(kw_return, "return", keyword_control, c_cpp)                                                 \
    F(kw_short, "short", keyword_type, c_cpp)                                                      \
    F(kw_signed, "signed", keyword_type, c_cpp)                                                    \
    F(kw_sizeof, "sizeof", keyword, c_cpp)                                                         \
    F(kw_static, "static", keyword, c_cpp)                                                         \
    F(kw_static_assert, "static_assert", keyword, c_cpp)                                           \
    F(kw_static_cast, "static_cast", keyword, cpp)                                                 \
    F(kw_struct, "struct", keyword, c_cpp)                                                         \
    F(kw_template, "template", keyword, cpp)                                                       \
    F(kw_this, "this", this_, cpp)                                                                 \
    F(kw_thread_local, "thread_local", keyword, c_cpp)                                             \
    F(kw_throw, "throw", keyword, cpp)                                                             \
    F(kw_trivially_relocatable_if_eligible, "trivially_relocatable_if_eligible", keyword, cpp)     \
    F(kw_true, "true", bool_, c_cpp)                                                               \
    F(kw_try, "try", keyword, cpp)                                                                 \
    F(kw_typedef, "typedef", keyword, c_cpp)                                                       \
    F(kw_typeid, "typeid", keyword, cpp)                                                           \
    F(kw_typename, "typename", keyword, cpp)                                                       \
    F(kw_typeof, "typeof", keyword, c)                                                             \
    F(kw_typeof_unqual, "typeof_unqual", keyword, c)                                               \
    F(kw_union, "union", keyword, c_cpp)                                                           \
    F(kw_unsigned, "unsigned", keyword_type, c_cpp)                                                \
    F(kw_using, "using", keyword, cpp)                                                             \
    F(kw_virtual, "virtual", keyword, cpp)                                                         \
    F(kw_void, "void", keyword_type, c_cpp)                                                        \
    F(kw_volatile, "volatile", keyword, c_cpp)                                                     \
    F(kw_wchar_t, "wchar_t", keyword_type, cpp)                                                    \
    F(kw_while, "while", keyword_control, c_cpp)                                                   \
    F(kw_xor, "xor", keyword, c_cpp)                                                               \
    F(kw_xor_eq, "xor_eq", keyword, c_cpp)                                                         \
    F(left_brace, "{", sym_brace, c_cpp)                                                           \
    F(pipe, "|", sym_op, c_cpp)                                                                    \
    F(pipe_eq, "|=", sym_op, c_cpp)                                                                \
    F(pipe_pipe, "||", sym_op, c_cpp)                                                              \
    F(right_brace, "}", sym_brace, c_cpp)                                                          \
    F(tilde, "~", sym_op, c_cpp)

#define ULIGHT_CPP_TOKEN_ENUM_ENUMERATOR(id, code, highlight, strict) id,

enum struct Token_Type : Underlying { //
    ULIGHT_CPP_TOKEN_ENUM_DATA(ULIGHT_CPP_TOKEN_ENUM_ENUMERATOR)
};

inline constexpr auto cpp_token_type_count = std::size_t(Token_Type::kw_xor_eq) + 1;

/// @brief Returns the in-code representation of `type`.
/// For example, if `type` is `plus`, returns `"+"`.
/// If `type` is invalid, returns an empty string.
[[nodiscard]]
std::u8string_view cpp_token_type_code(Token_Type type) noexcept;

/// @brief Equivalent to `cpp_token_type_code(type).length()`.
[[nodiscard]]
std::size_t cpp_token_type_length(Token_Type type) noexcept;

[[nodiscard]]
Highlight_Type cpp_token_type_highlight(Token_Type type) noexcept;

[[nodiscard]]
Feature_Source cpp_token_type_source(Token_Type type) noexcept;

[[nodiscard]]
std::optional<Token_Type> cpp_token_type_by_code(std::u8string_view code) noexcept;

/// @brief Matches zero or more characters for which `is_cpp_whitespace` is `true`.
[[nodiscard]]
std::size_t match_whitespace(std::u8string_view str);

/// @brief Matches zero or more characters for which `is_cpp_whitespace` is `false`.
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

/// @brief Returns a match for a C89-style block comment at the start of `str`, if any.
[[nodiscard]]
Comment_Result match_block_comment(std::u8string_view str) noexcept;

/// @brief Returns the length of a C99-style line comment at the start of `str`, if any.
/// Returns zero if there is no line comment.
/// In any case, the length does not include the terminating newline character,
/// but it does include intermediate newlines "escaped" via backslash.
[[nodiscard]]
std::size_t match_line_comment(std::u8string_view str) noexcept;

[[nodiscard]]
std::size_t match_preprocessing_directive(std::u8string_view str, Lang c_or_cpp);

/** @brief Type of https://eel.is/c++draft/lex.icon#nt:integer-literal */
enum struct Integer_Literal_Type : Underlying {
    /** @brief *binary-literal* */
    binary,
    /** @brief *octal-literal* */
    octal,
    /** @brief *decimal-literal* */
    decimal,
    /** @brief *hexadecimal-literal* */
    hexadecimal,
};

enum struct Literal_Match_Status : Underlying {
    /// @brief Successful match.
    ok,
    /// @brief The literal has no digits.
    no_digits,
    /// @brief The literal starts with an integer prefix `0x` or `0b`, but does not have any digits
    /// following it.
    no_digits_following_prefix
};

struct Literal_Match_Result {
    /// @brief The status of a literal match.
    /// If `status == ok`, matching succeeded.
    Literal_Match_Status status;
    /// @brief The length of the matched literal.
    /// If `status == ok`, `length` is the length of the matched literal.
    /// If `status == no_digits_following_prefix`, it is the length of the prefix.
    /// Otherwise, it is zero.
    std::size_t length;
    /// @brief The type of the matched literal.
    /// If `status == no_digits`, the value is value-initialized.
    Integer_Literal_Type type;

    [[nodiscard]] operator bool() const
    {
        return status == Literal_Match_Status::ok;
    }
};

/// @brief Matches a literal at the beginning of the given string.
/// This includes any prefix such as `0x`, `0b`, or `0` and all the following digits.
/// @param str the string which may contain a literal at the start
/// @return The match or an error.
[[nodiscard]]
Literal_Match_Result match_integer_literal(std::u8string_view str) noexcept;

/// @brief Matches the regex `\\.?[0-9]('?[0-9a-zA-Z_]|[eEpP][+-]|\\.)*`
/// at the start of `str`.
/// Returns `0` if it couldn't be matched.
///
/// A https://eel.is/c++draft/lex.ppnumber#nt:pp-number in C++ is a superset of
/// *integer-literal* and *floating-point-literal*,
/// and also includes malformed numbers like `1e+3p-55` that match neither of those,
/// but are still considered a single token.
///
/// pp-numbers are converted into a single token in phase
/// https://eel.is/c++draft/lex.phases#1.7
[[nodiscard]]
std::size_t match_pp_number(std::u8string_view str);

/// @brief Matches a C++
/// *[identifier](https://eel.is/c++draft/lex.name#nt:identifier)*
/// at the start of `str`
/// and returns its length.
/// If none could be matched, returns zero.
[[nodiscard]]
std::size_t match_identifier(std::u8string_view str);

enum struct Escape_Type : Underlying {
    /// @brief *simple-escape-sequence*
    simple,
    /// @brief *octal-escape-sequence*
    octal,
    /// @brief *hexadecimal-escape-sequence*
    hexadecimal,
    /// @brief *conditional-escape-sequence*
    conditional,
    /// @brief *universal-character-name*
    universal,
    /// @brief  `\\` followed by optional whitespace and a newline character.
    /// These are not considered *escape-sequence*s grammatically,
    /// but would have been preprocessed into a single space character in an earlier
    /// translation phase.
    newline,
};

struct Escape_Result {
    /// @brief The length of the escape sequence, in code units.
    std::size_t length;
    /// @brief The type of escape sequence.
    Escape_Type type;
    /// @brief If `true`, the escape sequence was recognized,
    /// but its contents are not valid.
    /// For example, there can be unterminated `\\o{...` escapes,
    /// `\\u{...}` escapes whose contents are not valid,
    /// `\\U` escapes where one of the following 8 characters is not a hexadecimal digit,
    /// and other such cases.
    bool erroneous = false;

    [[nodiscard]]
    constexpr explicit operator bool() const
    {
        return length != 0;
    }

    [[nodiscard]]
    friend constexpr bool operator==(Escape_Result, Escape_Result)
        = default;
};

/// @brief Matches a C++
/// *[escape-sequence](https://eel.is/c++draft/lex.literal#nt:escape-sequence)*,
/// *[universal-character-name](https://eel.is/c++draft/lex.universal.char#nt:universal-character-name)*,
/// or "newline escape" at the start of `str`,
// and returns its length. If none could be matched,
/// returns zero.
[[nodiscard]]
Escape_Result match_escape_sequence(std::u8string_view str);

/// @brief Matches a C or C++
/// *[preprocessing-op-or-punc](https://eel.is/c++draft/lex#nt:preprocessing-op-or-punc)*
/// at the start of `str`.
[[nodiscard]]
std::optional<Token_Type> match_preprocessing_op_or_punc(std::u8string_view str, Lang c_or_cpp);

} // namespace ulight::cpp

#endif
