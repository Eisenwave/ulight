#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory_resource>
#include <optional>
#include <string_view>
#include <vector>

#include "ulight/impl/buffer.hpp"
#include "ulight/impl/highlight.hpp"
#include "ulight/impl/parse_utils.hpp"
#include "ulight/ulight.hpp"

#include "ulight/impl/assert.hpp"
#include "ulight/impl/unicode.hpp"

#include "ulight/impl/lang/cpp.hpp"
#include "ulight/impl/lang/cpp_chars.hpp"

namespace ulight {

namespace cpp {

#define ULIGHT_CPP_TOKEN_TYPE_U8_CODE(id, code, highlight, source) u8##code,
#define ULIGHT_CPP_TOKEN_TYPE_LENGTH(id, code, highlight, source) (sizeof(u8##code) - 1),
#define ULIGHT_CPP_TOKEN_HIGHLIGHT_TYPE(id, code, highlight, source) (Highlight_Type::highlight),
#define ULIGHT_CPP_TOKEN_TYPE_FEATURE_SOURCE(id, code, highlight, source) (Feature_Source::source),

namespace {

inline constexpr std::u8string_view token_type_codes[] {
    ULIGHT_CPP_TOKEN_ENUM_DATA(ULIGHT_CPP_TOKEN_TYPE_U8_CODE)
};

static_assert(std::ranges::is_sorted(token_type_codes));

inline constexpr unsigned char token_type_lengths[] {
    ULIGHT_CPP_TOKEN_ENUM_DATA(ULIGHT_CPP_TOKEN_TYPE_LENGTH)
};

inline constexpr Highlight_Type token_type_highlights[] {
    ULIGHT_CPP_TOKEN_ENUM_DATA(ULIGHT_CPP_TOKEN_HIGHLIGHT_TYPE)
};

inline constexpr Feature_Source token_type_sources[] {
    ULIGHT_CPP_TOKEN_ENUM_DATA(ULIGHT_CPP_TOKEN_TYPE_FEATURE_SOURCE)
};

} // namespace

/// @brief Returns the in-code representation of `type`.
/// For example, if `type` is `plus`, returns `"+"`.
/// If `type` is invalid, returns an empty string.
[[nodiscard]]
std::u8string_view cpp_token_type_code(Token_Type type) noexcept
{
    return token_type_codes[std::size_t(type)];
}

/// @brief Equivalent to `cpp_token_type_code(type).length()`.
[[nodiscard]]
std::size_t cpp_token_type_length(Token_Type type) noexcept
{
    return token_type_lengths[std::size_t(type)];
}

[[nodiscard]]
Highlight_Type cpp_token_type_highlight(Token_Type type) noexcept
{
    return token_type_highlights[std::size_t(type)];
}

[[nodiscard]]
Feature_Source cpp_token_type_source(Token_Type type) noexcept
{
    return token_type_sources[std::size_t(type)];
}

[[nodiscard]]
std::optional<Token_Type> cpp_token_type_by_code(std::u8string_view code) noexcept
{
    const std::u8string_view* const result = std::ranges::lower_bound(token_type_codes, code);
    if (result == std::end(token_type_codes) || *result != code) {
        return {};
    }
    return Token_Type(result - token_type_codes);
}

std::size_t match_whitespace(std::u8string_view str)
{
    constexpr auto predicate = [](char8_t c) { return is_cpp_whitespace(c); };
    return std::size_t(std::ranges::find_if_not(str, predicate) - str.begin());
}

std::size_t match_non_whitespace(std::u8string_view str)
{
    constexpr auto predicate = [](char8_t c) { return is_cpp_whitespace(c); };
    return std::size_t(std::ranges::find_if(str, predicate) - str.begin());
}

namespace {

[[nodiscard]]
std::size_t match_newline_escape(std::u8string_view str) noexcept
{
    // https://eel.is/c++draft/lex.phases#1.2
    // > Each sequence of a backslash character (\)
    // > immediately followed by zero or more whitespace characters other than new-line
    // > followed by a new-line character is deleted,
    // > splicing physical source lines to form logical source lines.

    if (!str.starts_with(u8'\\')) {
        return 0;
    }
    std::size_t length = 1;
    for (; length < str.length(); ++length) {
        if (str[length] == u8'\n') {
            return length + 1;
        }
        if (!is_cpp_whitespace(str[length])) {
            return 0;
        }
    }
    return 0;
}

} // namespace

// NOLINTNEXTLINE(bugprone-exception-escape)
std::size_t match_line_comment(std::u8string_view s) noexcept
{
    if (!s.starts_with(u8"//")) {
        return {};
    }

    std::size_t length = 2;

    while (length < s.length()) {
        const auto remainder = s.substr(length);
        const bool terminated = remainder.starts_with(u8'\n') || remainder.starts_with(u8"\r\n");
        if (terminated) {
            return length;
        }
        if (const std::size_t escape = match_newline_escape(remainder)) {
            length += escape;
        }
        else {
            ++length;
        }
    }

    return length;
}

std::size_t match_preprocessing_directive(std::u8string_view s, Lang c_or_cpp)
{
    ULIGHT_ASSERT(c_or_cpp == Lang::c || c_or_cpp == Lang::cpp);

    const std::optional<Token_Type> first_token = match_preprocessing_op_or_punc(s, c_or_cpp);
    if (first_token != Token_Type::pound && first_token != Token_Type::pound_alt) {
        return {};
    }

    std::size_t length = cpp_token_type_length(*first_token);

    while (length < s.length()) {
        const auto remainder = s.substr(length);
        const bool terminated = remainder.starts_with(u8'\n') //
            || remainder.starts_with(u8"\r\n") //
            || remainder.starts_with(u8"//") //
            || remainder.starts_with(u8"/*");
        if (terminated) {
            return length;
        }
        if (const std::size_t escape = match_newline_escape(remainder)) {
            length += escape;
        }
        else {
            ++length;
        }
    }
    return length;
}

Comment_Result match_block_comment(std::u8string_view s) noexcept
{
    if (!s.starts_with(u8"/*")) {
        return {};
    }
    // naive: nesting disallowed, but line comments can be nested in block comments
    const std::size_t end = s.find(u8"*/", 2);
    if (end == std::string_view::npos) {
        return Comment_Result { .length = s.length(), .is_terminated = false };
    }
    return Comment_Result { .length = end + 2, .is_terminated = true };
}

Literal_Match_Result match_integer_literal // NOLINT(bugprone-exception-escape)
    (std::u8string_view s) noexcept
{
    if (s.empty() || !is_ascii_digit(s[0])) {
        return { Literal_Match_Status::no_digits, 0, {} };
    }
    if (s.starts_with(u8"0b")) {
        const std::size_t digits = match_digits(s.substr(2), 2);
        if (digits == 0) {
            return { Literal_Match_Status::no_digits_following_prefix, 2,
                     Integer_Literal_Type::binary };
        }
        return { Literal_Match_Status::ok, digits + 2, Integer_Literal_Type::binary };
    }
    if (s.starts_with(u8"0x")) {
        const std::size_t digits = match_digits(s.substr(2), 16);
        if (digits == 0) {
            return { Literal_Match_Status::no_digits_following_prefix, 2,
                     Integer_Literal_Type::hexadecimal };
        }
        return { Literal_Match_Status::ok, digits + 2, Integer_Literal_Type::hexadecimal };
    }
    if (s[0] == '0') {
        const std::size_t digits = match_digits(s, 8);
        return { Literal_Match_Status::ok, digits,
                 digits == 1 ? Integer_Literal_Type::decimal : Integer_Literal_Type::octal };
    }
    const std::size_t digits = match_digits(s, 10);

    return { Literal_Match_Status::ok, digits, Integer_Literal_Type::decimal };
}

namespace {

[[nodiscard]]
bool is_identifier_start_likely_ascii(char32_t c)
{
    if (is_ascii(c)) [[likely]] {
        return is_cpp_ascii_identifier_start(c);
    }
    return is_cpp_identifier_start(c);
}

[[nodiscard]]
bool is_identifier_continue_likely_ascii(char32_t c)
{
    if (is_ascii(c)) [[likely]] {
        return is_cpp_ascii_identifier_continue(c);
    }
    return is_cpp_identifier_continue(c);
}

} // namespace

std::size_t match_pp_number(const std::u8string_view str)
{
    std::size_t length = 0;
    // "." digit
    if (str.length() >= 2 && str[0] == u8'.' && is_ascii_digit(str[1])) {
        length += 2;
    }
    // digit
    else if (!str.empty() && is_ascii_digit(str[0])) {
        length += 1;
    }
    else {
        return length;
    }

    while (length < str.size()) {
        switch (str[length]) {
        // pp-number "'" digit
        // pp-number "'" nondigit
        case u8'\'': {
            if (length + 1 < str.size() && is_cpp_ascii_identifier_continue(str[length + 1])) {
                length += 2;
                break;
            }
            return length;
        }
        case u8'e':
        case u8'E':
        case u8'p':
        case u8'P': {
            // pp-number "e" sign
            // pp-number "E" sign
            // pp-number "p" sign
            // pp-number "P" sign
            if (length + 1 < str.size() && (str[length + 1] == u8'-' || str[length + 1] == u8'+')) {
                length += 2;
            }
            // pp-number identifier-continue
            else {
                ++length;
            }
            break;
        }
        // pp-number "."
        case u8'.': {
            ++length;
            break;
        }
        // pp-number identifier-continue
        default: {
            const std::u8string_view remainder = str.substr(length);
            const auto [code_point, units] = utf8::decode_and_length_or_throw(remainder);
            if (is_identifier_continue_likely_ascii(code_point)) {
                length += std::size_t(units);
                break;
            }
            return length;
        }
        }
    }

    return length;
}

std::size_t match_identifier(std::u8string_view str)
{
    std::size_t length = 0;

    if (!str.empty()) {
        const auto [code_point, units] = utf8::decode_and_length_or_throw(str);
        if (!is_identifier_start_likely_ascii(code_point)) {
            return length;
        }
        str.remove_prefix(std::size_t(units));
        length += std::size_t(units);
    }

    while (!str.empty()) {
        const auto [code_point, units] = utf8::decode_and_length_or_throw(str);
        if (!is_identifier_continue_likely_ascii(code_point)) {
            return length;
        }
        str.remove_prefix(std::size_t(units));
        length += std::size_t(units);
    }

    return length;
}

Escape_Result match_escape_sequence(std::u8string_view str)
{
    // https://eel.is/c++draft/lex.literal#nt:escape-sequence
    if (!str.starts_with(u8'\\') || str.length() < 2) {
        return {};
    }

    const auto* const data_end = str.data() + str.length();
    switch (str[1]) {
    // https://eel.is/c++draft/lex.literal#nt:simple-escape-sequence-char
    case u8'\'':
    case u8'"':
    case u8'?':
    case u8'\\':
    case u8'a':
    case u8'b':
    case u8'f':
    case u8'n':
    case u8'r':
    case u8't':
    case u8'v': {
        return { .length = 2, .type = Escape_Type::simple };
    }
    case u8' ':
    case u8'\t':
    case u8'\v':
    case u8'\f':
    case u8'\r': {
        if (const std::size_t length = match_newline_escape(str)) {
            return { .length = length, .type = Escape_Type::newline };
        }
        return { .length = 2, .type = Escape_Type::newline, .erroneous = true };
    }
    case u8'\n': {
        return { .length = 2, .type = Escape_Type::newline };
    }
    case u8'u': {
        // https://eel.is/c++draft/lex.universal.char#nt:universal-character-name
        if (str.length() >= 3 && str[2] == u8'{') {
            const auto* const end = std::ranges::find_if(str.data() + 3, data_end, [](char8_t c) {
                return c == u8'}' || !is_ascii_hex_digit(c);
            });
            if (end == data_end || *end != u8'}') {
                return { .length = 3, .type = Escape_Type::universal, .erroneous = true };
            }
            return { .length = std::size_t(end - str.data() + 1), .type = Escape_Type::universal };
        }
        const auto* const end = std::ranges::find_if_not(
            str.data() + 2, str.data() + std::min(str.length(), 6uz),
            [](char8_t c) { return is_ascii_hex_digit(c); }
        );
        return { .length = 6, .type = Escape_Type::universal, .erroneous = end - str.data() != 6 };
    }
    case u8'U': {
        const auto* const end = std::ranges::find_if_not(
            str.data() + 2, str.data() + std::min(str.length(), 10uz),
            [](char8_t c) { return is_ascii_hex_digit(c); }
        );
        const auto length = std::size_t(end - str.data());
        return { .length = length, .type = Escape_Type::universal, .erroneous = length != 10 };
    }
    case u8'N': {
        // https://eel.is/c++draft/lex.universal.char#nt:named-universal-character
        if (str.length() >= 3 && str[2] == u8'{') {
            const std::size_t end = str.find_first_of(u8"}\n");
            if (end == std::u8string_view::npos || str[end] != u8'}') {
                return { .length = 3, .type = Escape_Type::universal, .erroneous = true };
            }
            return { .length = end + 1, .type = Escape_Type::universal };
        }
        return { .length = 2, .type = Escape_Type::universal, .erroneous = true };
    }
    case u8'x': {
        // https://eel.is/c++draft/lex.literal#nt:hexadecimal-escape-sequence
        if (str.length() >= 3 && str[2] == u8'{') {
            const auto* const end = std::ranges::find_if(str.data() + 3, data_end, [](char8_t c) {
                return c == u8'}' || !is_ascii_hex_digit(c);
            });
            if (end == data_end || *end != u8'}') {
                return { .length = 3, .type = Escape_Type::hexadecimal, .erroneous = true };
            }
            return { .length = std::size_t(end - str.data() + 1),
                     .type = Escape_Type::hexadecimal };
        }
        const auto* const end = std::ranges::find_if_not(str.data() + 2, data_end, [](char8_t c) {
            return is_ascii_hex_digit(c);
        });
        if (end == data_end) {
            return {};
        }
        return { .length = std::size_t(end - str.data()), .type = Escape_Type::hexadecimal };
    }
    case u8'o': {
        // https://eel.is/c++draft/lex.literal#nt:octal-escape-sequence
        if (str.length() >= 3 && str[2] == u8'{') {
            const auto* const end = std::ranges::find_if(str.data() + 3, data_end, [](char8_t c) {
                return c == u8'}' || !is_ascii_octal_digit(c);
            });
            if (end == data_end || *end != u8'}') {
                return { .length = 3, .type = Escape_Type::octal, .erroneous = true };
            }
            return { .length = std::size_t(end - str.data() + 1), .type = Escape_Type::octal };
        }
        return { .length = 2, .type = Escape_Type::octal, .erroneous = true };
    }
    case u8'0':
    case u8'1':
    case u8'2':
    case u8'3':
    case u8'4':
    case u8'5':
    case u8'6':
    case u8'7': {
        // https://eel.is/c++draft/lex.literal#nt:octal-escape-sequence
        const auto* const limit = str.data() + std::min(str.length(), 4uz);
        const auto* const end = std::ranges::find_if_not(str.data() + 2, limit, [](char8_t c) {
            return is_ascii_octal_digit(c);
        });
        return { .length = std::size_t(end - str.data()), .type = Escape_Type::octal };
    }

    default: {
        return { .length = 2,
                 .type = Escape_Type::conditional,
                 .erroneous = !is_cpp_basic(str[1]) };
    }
    }
    ULIGHT_ASSERT_UNREACHABLE(u8"Some switch case should have handled it.");
}

namespace {

[[nodiscard]]
constexpr bool is_d_char(char8_t c)
{
    return is_ascii(c) && !is_cpp_whitespace(c) && c != u8'(' && c != u8')' && c != '\\';
}

[[nodiscard]]
std::size_t match_d_char_sequence(std::u8string_view str)
{
    constexpr auto predicate = [](char8_t c) { return is_d_char(c); };
    return std::size_t(std::ranges::find_if_not(str, predicate) - str.begin());
}

} // namespace

std::optional<Token_Type> match_preprocessing_op_or_punc(std::u8string_view str, Lang c_or_cpp)
{
    ULIGHT_ASSERT(c_or_cpp == Lang::c || c_or_cpp == Lang::cpp);
    const bool is_cpp = c_or_cpp == Lang::cpp;

    using enum Token_Type;
    if (str.empty()) {
        return {};
    }
    switch (str[0]) {
    case u8'#': return str.starts_with(u8"##") ? pound_pound : pound;
    case u8'%':
        return str.starts_with(u8"%:%:") ? pound_pound_alt
            : str.starts_with(u8"%:")    ? pound_alt
            : str.starts_with(u8"%=")    ? percent_eq
            : str.starts_with(u8"%>")    ? right_square_alt
                                         : percent;
    case u8'{': return left_brace;
    case u8'}': return right_brace;
    case u8'[': return left_square;
    case u8']': return right_square;
    case u8'(': return left_parens;
    case u8')': return right_parens;
    case u8'<': {
        // https://eel.is/c++draft/lex.pptoken#4.2
        if (str.starts_with(u8"<::") && !str.starts_with(u8"<:::") && !str.starts_with(u8"<::>")) {
            return less;
        }
        return is_cpp && str.starts_with(u8"<=>") ? three_way
            : str.starts_with(u8"<<=")            ? less_less_eq
            : str.starts_with(u8"<=")             ? less_eq
            : str.starts_with(u8"<<")             ? less_less
            : str.starts_with(u8"<%")             ? left_brace_alt
            : str.starts_with(u8"<:")             ? left_square_alt
                                                  : less;
    }
    case u8';': return semicolon;
    case u8':':
        return str.starts_with(u8":>")          ? right_square_alt //
            : is_cpp && str.starts_with(u8"::") ? scope
                                                : colon;
    case u8'.':
        return str.starts_with(u8"...")         ? ellipsis
            : is_cpp && str.starts_with(u8".*") ? member_pointer_access
                                                : dot;
    case u8'?': return question;
    case u8'-': {
        return is_cpp && str.starts_with(u8"->*") ? member_arrow_access
            : str.starts_with(u8"-=")             ? minus_eq
            : str.starts_with(u8"->")             ? arrow
            : str.starts_with(u8"--")             ? minus_minus
                                                  : minus;
    }
    case u8'>':
        return str.starts_with(u8">>=") ? greater_greater_eq
            : str.starts_with(u8">=")   ? greater_eq
            : str.starts_with(u8">>")   ? greater_greater
                                        : greater;
    case u8'~': return tilde;
    case u8'!': return str.starts_with(u8"!=") ? exclamation_eq : exclamation;
    case u8'+':
        return str.starts_with(u8"++") ? plus_plus //
            : str.starts_with(u8"+=")  ? plus_eq
                                       : plus;

    case u8'*': return str.starts_with(u8"*=") ? asterisk_eq : asterisk;
    case u8'/': return str.starts_with(u8"/=") ? slash_eq : slash;
    case u8'^':
        return is_cpp && str.starts_with(u8"^^") ? caret_caret //
            : str.starts_with(u8"^=")            ? caret_eq
                                                 : caret;
    case u8'&':
        return str.starts_with(u8"&=") ? amp_eq //
            : str.starts_with(u8"&&")  ? amp_amp
                                       : amp;

    case u8'|':
        return str.starts_with(u8"|=") ? pipe_eq //
            : str.starts_with(u8"||")  ? pipe_pipe
                                       : pipe;
    case u8'=': return str.starts_with(u8"==") ? eq_eq : eq;
    case u8',': return comma;
    default: return {};
    }
}

namespace {

constexpr std::uint_fast8_t source_mask_all { 0b1111 };
constexpr std::uint_fast8_t source_mask_standard_cpp { 0b1100 };
constexpr std::uint_fast8_t source_mask_standard_c { 0b1010 };
constexpr std::uint_fast8_t source_mask_standard_c_ext { 0b1011 };

[[nodiscard]]
bool feature_in_mask(Feature_Source source, std::uint_fast8_t mask)
{
    return ((mask >> Underlying(source)) & 1) != 0;
}

[[nodiscard]]
bool feature_in_mask(Token_Type type, std::uint_fast8_t mask)
{
    return feature_in_mask(cpp_token_type_source(type), mask);
}

[[nodiscard]]
Highlight_Type usual_fallback_highlight(std::u8string_view id)
{
    return id.ends_with(u8"_t") ? Highlight_Type::id_type : Highlight_Type::id;
}

// Approximately implements highlighting based on C++ tokenization,
// as described in:
// https://eel.is/c++draft/lex.phases
// https://eel.is/c++draft/lex.pptoken
struct Highlighter {
private:
    Non_Owning_Buffer<Token>& out;
    std::u8string_view source;
    Lang c_or_cpp;
    const Highlight_Options& options;

    std::size_t index = 0;
    // We need to keep track of whether we're on a "fresh line" for preprocessing directives.
    // A line is fresh if we've not encountered anything but whitespace on it yet.
    // https://eel.is/c++draft/cpp#def:preprocessing_directive
    bool fresh_line = true;
    const std::uint_fast8_t feature_source_mask = //
        options.strict && c_or_cpp == Lang::c     ? source_mask_standard_c
        : options.strict && c_or_cpp == Lang::cpp ? source_mask_standard_cpp
        : c_or_cpp == Lang::c                     ? source_mask_standard_c_ext
                                                  : source_mask_all;

public:
    Highlighter(
        Non_Owning_Buffer<Token>& out,
        std::u8string_view source,
        Lang c_or_cpp,
        const Highlight_Options& options
    )
        : out { out }
        , source { source }
        , c_or_cpp { c_or_cpp }
        , options { options }
    {
        ULIGHT_ASSERT(c_or_cpp == Lang::c || c_or_cpp == Lang::cpp);
    }

private:
    void emit(std::size_t begin, std::size_t length, Highlight_Type type)
    {
        ULIGHT_DEBUG_ASSERT(length != 0);
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
    }

    void emit_and_advance(std::size_t length, Highlight_Type type)
    {
        emit(index, length, type);
        index += length;
    }

    void advance(std::size_t length)
    {
        index += length;
    }

    [[nodiscard]]
    std::u8string_view remainder() const
    {
        return source.substr(index);
    }

public:
    bool operator()()
    {
        while (index < source.size()) {
            const bool any_matched = expect_whitespace() //
                || expect_line_comment() //
                || expect_block_comment() //
                || expect_string_literal() //
                || expect_character_literal() //
                || expect_pp_number() //
                || expect_identifier_or_keyword(usual_fallback_highlight) //
                || expect_preprocessing_op_or_punc() //
                || expect_non_whitespace();
            ULIGHT_ASSERT(any_matched);
        }
        return true;
    }

    bool expect_whitespace()
    {
        if (const std::size_t white_length = match_whitespace(remainder())) {
            fresh_line |= remainder().substr(0, white_length).contains(u8'\n');
            index += white_length;
            return true;
        }
        return false;
    }

    bool expect_line_comment()
    {
        if (const std::size_t line_comment_length = match_line_comment(remainder())) {
            emit_and_advance(2, Highlight_Type::comment_delim);
            if (line_comment_length > 2) {
                emit_and_advance(line_comment_length - 2, Highlight_Type::comment);
            }
            fresh_line = true;
            return true;
        }
        return false;
    }

    bool expect_block_comment()
    {
        if (const Comment_Result block_comment = match_block_comment(remainder())) {
            const std::size_t terminator_length = 2 * std::size_t(block_comment.is_terminated);
            emit(index, 2, Highlight_Type::comment_delim); // /*
            const std::size_t content_length = block_comment.length - 2 - terminator_length;
            if (content_length != 0) {
                emit(index + 2, content_length, Highlight_Type::comment);
            }
            if (block_comment.is_terminated) {
                emit(index + block_comment.length - 2, 2, Highlight_Type::comment_delim); // */
            }
            advance(block_comment.length);
            return true;
        }
        return false;
    }

    bool expect_character_literal()
    {
        // https://eel.is/c++draft/lex#nt:character-literal
        constexpr char8_t quote_char = u8'\'';

        const std::size_t prefix_length = match_identifier(remainder());
        if (source[index + prefix_length] != quote_char) {
            return false;
        }

        if (prefix_length != 0) {
            const auto prefix = remainder().substr(0, prefix_length);
            const auto prefix_highlight
                = prefix == u8"u8" || prefix == u8"u" || prefix == u8"U" || prefix == u8"L"
                ? Highlight_Type::string_decor
                : Highlight_Type::error;
            emit_and_advance(prefix_length, prefix_highlight);
        }
        ULIGHT_DEBUG_ASSERT(source[index] == quote_char);
        emit_and_advance(1, Highlight_Type::string_delim);

        consume_char_sequence_and_suffix(quote_char);
        return true;
    }

    bool expect_string_literal()
    {
        // https://eel.is/c++draft/lex.string#:string-literal
        constexpr char8_t quote_char = u8'"';

        const std::size_t prefix_length = match_identifier(remainder());
        if (source[index + prefix_length] != quote_char) {
            return false;
        }

        bool is_raw = false;
        if (prefix_length != 0) {
            const auto prefix = remainder().substr(0, prefix_length);
            const auto prefix_highlight = //
                prefix == u8"u8" || prefix == u8"u" || prefix == u8"U" || prefix == u8"L" || //
                    prefix == u8"u8R" || prefix == u8"uR" || prefix == u8"UR" || prefix == u8"LR"
                    || //
                    prefix == u8"R"
                ? Highlight_Type::string_decor
                : Highlight_Type::error;
            emit_and_advance(prefix_length, prefix_highlight);
            is_raw = prefix.ends_with(u8'R');
        }
        ULIGHT_DEBUG_ASSERT(source[index] == quote_char);

        if (is_raw) {
            consume_raw_string_and_suffix();
        }
        else {
            emit_and_advance(1, Highlight_Type::string_delim);
            consume_char_sequence_and_suffix(quote_char);
        }

        return true;
    }

    void consume_char_sequence_and_suffix(char8_t quote_char)
    {
        std::size_t chars_length = 0;
        const auto flush_chars = [&] {
            if (chars_length != 0) {
                ULIGHT_DEBUG_ASSERT(chars_length <= index);
                emit(index - chars_length, chars_length, Highlight_Type::string);
                chars_length = 0;
            }
        };

        while (index < source.size()) {
            if (source[index] == quote_char) {
                flush_chars();
                emit_and_advance(1, Highlight_Type::string_delim);
                expect_identifier_or_keyword([](std::u8string_view) {
                    return Highlight_Type::string_decor;
                });
                fresh_line = false;
                return;
            }
            if (source[index] == u8'\\') {
                if (const Escape_Result escape = match_escape_sequence(remainder())) {
                    if (escape.type == Escape_Type::newline) {
                        chars_length += escape.length;
                        advance(escape.length);
                    }
                    else {
                        flush_chars();
                        emit_and_advance(
                            escape.length,
                            escape.erroneous ? Highlight_Type::error : Highlight_Type::escape
                        );
                    }
                }
                else {
                    flush_chars();
                    emit_and_advance(1, Highlight_Type::error);
                }
                continue;
            }
            if (source[index] == u8'\r' || source[index] == u8'\n') {
                flush_chars();
                fresh_line = true;
                return;
            }
            const auto code_units = std::size_t(utf8::sequence_length(source[index], 1));
            chars_length += code_units;
            advance(code_units);
        }

        // This point is only reached when the literal is unterminated.
        // We still want the literal to be highlighted.
        flush_chars();
        fresh_line = false;
    }

    void consume_raw_string_and_suffix()
    {
        // https://eel.is/c++draft/lex.string#nt:raw-string
        std::u8string_view rem = remainder();
        ULIGHT_DEBUG_ASSERT(rem.starts_with(u8'"'));

        fresh_line = false;
        const std::size_t d_char_sequence_length = match_d_char_sequence(rem.substr(1));
        const std::u8string_view d_char_sequence = rem.substr(1, d_char_sequence_length);

        if (rem.length() <= d_char_sequence_length + 1) {
            emit_and_advance(d_char_sequence_length + 1, Highlight_Type::error);
            return;
        }
        if (rem[d_char_sequence_length + 1] != u8'(') {
            emit_and_advance(d_char_sequence_length + 2, Highlight_Type::error);
            return;
        }
        rem.remove_prefix(d_char_sequence_length + 2);
        emit_and_advance(d_char_sequence_length + 2, Highlight_Type::string_delim);

        const auto match_raw_terminator = [&](std::u8string_view str) {
            return str.starts_with(u8')') //
                && str.substr(1).starts_with(d_char_sequence) //
                && str.substr(1 + d_char_sequence_length).starts_with(u8'"');
        };

        std::size_t raw_length = 0;
        for (; raw_length < rem.size(); ++raw_length) {
            if (!match_raw_terminator(rem.substr(raw_length))) {
                continue;
            }
            if (raw_length != 0) {
                emit_and_advance(raw_length, Highlight_Type::string);
            }
            emit_and_advance(d_char_sequence_length + 2, Highlight_Type::string_delim);
            return;
        }
        // Unterminated raw string, possibly empty in the case of trailing R"(
        if (raw_length != 0) {
            emit_and_advance(raw_length, Highlight_Type::string);
        }
    }

    bool expect_pp_number()
    {
        // While it may seem unnecessary to match pp-numbers first,
        // we want a strict guarantee that the highlighter progresses by a number,
        // even if it doesn't match any valid literal.
        const std::u8string_view rem = remainder();
        if (const std::size_t number_length = match_pp_number(rem)) {
            const std::size_t old_index = index;
            highlight_pp_number(rem.substr(0, number_length));
            ULIGHT_DEBUG_ASSERT(old_index + number_length == index);
            fresh_line = false;
            return true;
        }
        return false;
    }

    void highlight_pp_number(std::u8string_view pp_number)
    {
        ULIGHT_ASSERT(!pp_number.empty());

        const bool is_hex = pp_number.starts_with(u8"0x");
        const bool is_binary = pp_number.starts_with(u8"0b");

        if (is_hex || is_binary) {
            emit_and_advance(2, Highlight_Type::number_decor);
            pp_number.remove_prefix(2);
        }

        std::size_t digits = 0;
        const auto flush_digits = [&] {
            if (digits != 0) {
                emit_and_advance(digits, Highlight_Type::number);
                digits = 0;
            }
        };

        while (!pp_number.empty()) {
            switch (const char8_t c = pp_number[0]) {
            // https://eel.is/c++draft/lex.fcon#nt:digit-sequence
            case u8'\'': {
                flush_digits();
                emit_and_advance(1, Highlight_Type::number_delim);
                pp_number.remove_prefix(1);
                break;
            }
            // https://eel.is/c++draft/lex.fcon#nt:exponent-part
            case u8'e':
            case u8'E': {
                if (is_hex) {
                    pp_number.remove_prefix(1);
                    ++digits;
                    break;
                }
                flush_digits();
                const bool is_exponent = pp_number.size() >= 2
                    && (pp_number[1] == u8'-' || pp_number[1] == u8'+'
                        || is_ascii_digit(pp_number[1]));
                if (is_exponent) {
                    emit_and_advance(1, Highlight_Type::number_delim);
                    pp_number.remove_prefix(1);
                    break;
                }
                emit_and_advance(pp_number.length(), Highlight_Type::number_decor);
                return;
            }
            case u8'p':
            case u8'P': {
                flush_digits();
                const bool is_exponent = pp_number.size() >= 2 && is_hex
                    && (pp_number[1] == u8'-' || pp_number[1] == u8'+'
                        || is_ascii_digit(pp_number[1]));
                if (is_exponent) {
                    emit_and_advance(1, Highlight_Type::number_delim);
                    pp_number.remove_prefix(1);
                    break;
                }
                emit_and_advance(pp_number.length(), Highlight_Type::number_decor);
                return;
            }
            case u8'.': {
                flush_digits();
                emit_and_advance(1, Highlight_Type::number_delim);
                pp_number.remove_prefix(1);
                break;
            }
            case u8'+':
            case u8'-':
            case u8'0':
            case u8'1':
            case u8'2':
            case u8'3':
            case u8'4':
            case u8'5':
            case u8'6':
            case u8'7':
            case u8'8':
            case u8'9': {
                digits += 1;
                pp_number.remove_prefix(1);
                break;
            }
            default: {
                if (is_hex && is_ascii_hex_digit(c)) {
                    digits += 1;
                    pp_number.remove_prefix(1);
                    break;
                }
                flush_digits();
                emit_and_advance(pp_number.length(), Highlight_Type::number_decor);
                return;
            }
            }
        }
        flush_digits();
    }

    bool expect_identifier_or_keyword(Highlight_Type fallback_highlight(std::u8string_view))
    {
        const std::size_t id_length = match_identifier(remainder());
        if (id_length == 0) {
            return false;
        }
        const std::u8string_view id = remainder().substr(0, id_length);
        const std::optional<Token_Type> keyword = cpp_token_type_by_code(id);
        const auto highlight = keyword && feature_in_mask(*keyword, feature_source_mask)
            ? cpp_token_type_highlight(*keyword)
            : fallback_highlight(id);
        emit_and_advance(id_length, highlight);
        fresh_line = false;
        return true;
    }

    bool expect_preprocessing_op_or_punc()
    {
        if (const std::optional<Token_Type> op
            = match_preprocessing_op_or_punc(remainder(), c_or_cpp)) {
            const bool possible_directive = op == Token_Type::pound || op == Token_Type::pound_alt;
            if (fresh_line && possible_directive) {
                if (const std::size_t directive_length
                    = match_preprocessing_directive(remainder(), c_or_cpp)) {
                    emit_and_advance(directive_length, Highlight_Type::macro);
                    fresh_line = true;
                    return true;
                }
            }
            const std::size_t op_length = cpp_token_type_length(*op);
            const Highlight_Type op_highlight = cpp_token_type_highlight(*op);
            emit_and_advance(op_length, op_highlight);
            fresh_line = false;
            return true;
        }
        return false;
    }

    bool expect_non_whitespace()
    {
        if (const std::size_t non_white_length = match_non_whitespace(remainder())) {
            // Don't emit any highlighting.
            // To my understanding, this currently only matches backslashes at the end of the
            // line. We don't have a separate phase for these, so whatever, this seems fine.
            fresh_line = false;
            advance(non_white_length);
            return true;
        }
        return false;
    }
};

} // namespace
} // namespace cpp

bool highlight_c( //
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    std::pmr::memory_resource*,
    const Highlight_Options& options
)
{
    return cpp::Highlighter { out, source, Lang::c, options }();
}

bool highlight_cpp( //
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    std::pmr::memory_resource*,
    const Highlight_Options& options
)
{
    return cpp::Highlighter { out, source, Lang::cpp, options }();
}

} // namespace ulight
