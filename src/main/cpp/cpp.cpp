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
#include "ulight/impl/chars.hpp"
#include "ulight/impl/cpp.hpp"
#include "ulight/impl/unicode.hpp"

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

    std::size_t length = 2;

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
            }
            break;
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

Character_Literal_Result match_character_literal(std::u8string_view str)
{
    std::size_t length = 0;
    if (str.starts_with(u8"u8")) {
        length += 2;
    }
    else if (str.starts_with(u8'u') || str.starts_with(u8'U') || str.starts_with(u8'L')) {
        length += 1;
    }
    const std::size_t encoding_prefix_length = length;

    if (length >= str.size() || str[length] != u8'\'') {
        return {};
    }
    ++length;
    while (length < str.size()) {
        const std::u8string_view remainder = str.substr(length);
        const auto [code_point, units] = utf8::decode_and_length_or_throw(remainder);
        switch (code_point) {
        case '\'': {
            return { .length = length + 1,
                     .encoding_prefix_length = encoding_prefix_length,
                     .terminated = true };
        }
        case U'\\': {
            length += std::size_t(units) + 1;
            break;
        }
        case U'\n':
            return { .length = length,
                     .encoding_prefix_length = encoding_prefix_length,
                     .terminated = false };
        default: {
            length += std::size_t(units);
            break;
        }
        }
    }

    return { .length = length,
             .encoding_prefix_length = encoding_prefix_length,
             .terminated = false };
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

String_Literal_Result match_string_literal(std::u8string_view str)
{
    std::size_t length = 0;
    const auto expect = [&](char8_t c) {
        if (length < str.length() && str[length] == c) {
            ++length;
            return true;
        }
        return false;
    };

    if (str.starts_with(u8"u8")) {
        length += 2;
    }
    else {
        expect(u8'u') || expect(u8'U') || expect(u8'L');
    }
    const std::size_t encoding_prefix_length = length;

    if (length >= str.size()) {
        return {};
    }
    const bool raw = expect(u8'R');
    if (!expect(u8'"')) {
        return {};
    }
    if (raw) {
        const std::size_t d_char_sequence_length = match_d_char_sequence(str.substr(length));
        const std::u8string_view d_char_sequence = str.substr(length, d_char_sequence_length);
        length += d_char_sequence_length;

        if (!expect(u8'(')) {
            return {};
        }
        for (; length < str.size(); ++length) {
            if (str[length] == u8')' //
                && str.substr(1).starts_with(d_char_sequence) //
                && str.substr(1 + d_char_sequence_length).starts_with(u8'"')) {
                return { .length = length + d_char_sequence_length + 2,
                         .encoding_prefix_length = encoding_prefix_length,
                         .raw = true,
                         .terminated = true };
            }
        }
    }
    else {
        while (length < str.size()) {
            const std::u8string_view remainder = str.substr(length);
            const auto [code_point, units] = utf8::decode_and_length_or_throw(remainder);
            switch (code_point) {
            case '\"': {
                return { .length = length + 1,
                         .encoding_prefix_length = encoding_prefix_length,
                         .raw = raw,
                         .terminated = true };
            }
            case U'\\': {
                length += std::size_t(units) + 1;
                break;
            }
            case U'\n':
                return { .length = length,
                         .encoding_prefix_length = encoding_prefix_length,
                         .raw = raw,
                         .terminated = false };
            default: {
                length += std::size_t(units);
                break;
            }
            }
        }
    }

    return { .length = length,
             .encoding_prefix_length = encoding_prefix_length,
             .raw = raw,
             .terminated = false };
}

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
std::size_t
match_cpp_identifier_except_keywords(std::u8string_view str, std::uint_fast8_t source_mask)
{
    if (const std::size_t result = cpp::match_identifier(str)) {
        const std::optional<Token_Type> keyword = cpp_token_type_by_code(str.substr(0, result));
        if (keyword && feature_in_mask(*keyword, source_mask)) {
            return 0;
        }
        return result;
    }
    return 0;
}

void highlight_c_cpp( //
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    Lang c_or_cpp,
    const Highlight_Options& options
)
{
    ULIGHT_ASSERT(c_or_cpp == Lang::c || c_or_cpp == Lang::cpp);
    const std::uint_fast8_t feature_source_mask = //
        options.strict && c_or_cpp == Lang::c     ? source_mask_standard_c
        : options.strict && c_or_cpp == Lang::cpp ? source_mask_standard_cpp
        : c_or_cpp == Lang::c                     ? source_mask_standard_c_ext
                                                  : source_mask_all;

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
    // Approximately implements highlighting based on C++ tokenization,
    // as described in:
    // https://eel.is/c++draft/lex.phases
    // https://eel.is/c++draft/lex.pptoken

    std::size_t index = 0;
    // We need to keep track of whether we're on a "fresh line" for preprocessing directives.
    // A line is fresh if we've not encountered anything but whitespace on it yet.
    // https://eel.is/c++draft/cpp#def:preprocessing_directive
    bool fresh_line = true;

    while (index < source.size()) {
        const std::u8string_view remainder = source.substr(index);
        if (const std::size_t white_length = match_whitespace(remainder)) {
            fresh_line |= remainder.substr(0, white_length).contains(u8'\n');
            index += white_length;
            continue;
        }
        if (const std::size_t line_comment_length = match_line_comment(remainder)) {
            emit(index, 2, Highlight_Type::comment_delim);
            emit(index + 2, line_comment_length - 2, Highlight_Type::comment);
            fresh_line = true;
            index += line_comment_length;
            continue;
        }
        if (const Comment_Result block_comment = match_block_comment(remainder)) {
            const std::size_t terminator_length = 2 * std::size_t(block_comment.is_terminated);
            emit(index, 2, Highlight_Type::comment_delim); // /*
            emit(index + 2, block_comment.length - 2 - terminator_length, Highlight_Type::comment);
            if (block_comment.is_terminated) {
                emit(index + block_comment.length - 2, 2, Highlight_Type::comment_delim); // */
            }
            index += block_comment.length;
            continue;
        }
        if (const String_Literal_Result literal = match_string_literal(remainder)) {
            const std::size_t suffix_length = literal.terminated
                ? match_cpp_identifier_except_keywords(
                      remainder.substr(literal.length), feature_source_mask
                  )
                : 0;
            const std::size_t combined_length = literal.length + suffix_length;
            emit(index, combined_length, Highlight_Type::string);
            fresh_line = false;
            index += combined_length;
            continue;
        }
        if (const Character_Literal_Result literal = match_character_literal(remainder)) {
            const std::size_t suffix_length = literal.terminated
                ? match_cpp_identifier_except_keywords(
                      remainder.substr(literal.length), feature_source_mask
                  )
                : 0;
            const std::size_t combined_length = literal.length + suffix_length;
            emit(index, combined_length, Highlight_Type::string);
            fresh_line = false;
            index += combined_length;
            continue;
        }
        if (const std::size_t number_length = match_pp_number(remainder)) {
            emit(index, number_length, Highlight_Type::number);
            fresh_line = false;
            index += number_length;
            continue;
        }
        if (const std::size_t id_length = match_identifier(remainder)) {
            const std::optional<Token_Type> keyword
                = cpp_token_type_by_code(remainder.substr(0, id_length));
            const auto highlight = keyword && feature_in_mask(*keyword, feature_source_mask)
                ? cpp_token_type_highlight(*keyword)
                : Highlight_Type::id;
            emit(index, id_length, highlight);
            fresh_line = false;
            index += id_length;
            continue;
        }
        if (const std::optional<Token_Type> op
            = match_preprocessing_op_or_punc(remainder, c_or_cpp)) {
            const bool possible_directive = op == Token_Type::pound || op == Token_Type::pound_alt;
            if (fresh_line && possible_directive) {
                if (const std::size_t directive_length
                    = match_preprocessing_directive(remainder, c_or_cpp)) {
                    emit(index, directive_length, Highlight_Type::macro);
                    fresh_line = true;
                    index += directive_length;
                    continue;
                }
            }
            const std::size_t op_length = cpp_token_type_length(*op);
            const Highlight_Type op_highlight = cpp_token_type_highlight(*op);
            emit(index, op_length, op_highlight);
            fresh_line = false;
            index += op_length;
            continue;
        }
        if (const std::size_t non_white_length = match_non_whitespace(remainder)) {
            // Don't emit any highlighting.
            // To my understanding, this currently only matches backslashes at the end of the line.
            // We don't have a separate phase for these, so whatever, this seems fine.
            fresh_line = false;
            index += non_white_length;
            continue;
        }

        ULIGHT_ASSERT_UNREACHABLE(u8"Impossible state. One of the rules above should have matched."
        );
    }
}

} // namespace
} // namespace cpp

bool highlight_c( //
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    std::pmr::memory_resource*,
    const Highlight_Options& options
)
{
    cpp::highlight_c_cpp(out, source, Lang::c, options);
    return true;
}

bool highlight_cpp( //
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    std::pmr::memory_resource*,
    const Highlight_Options& options
)
{
    cpp::highlight_c_cpp(out, source, Lang::cpp, options);
    return true;
}

} // namespace ulight
