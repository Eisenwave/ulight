#include <cstddef>
#include <memory_resource>
#include <string_view>

#include "ulight/ulight.hpp"

#include "ulight/impl/assert.hpp"
#include "ulight/impl/buffer.hpp"
#include "ulight/impl/highlight.hpp"
#include "ulight/impl/highlighter.hpp"
#include "ulight/impl/strings.hpp"
#include "ulight/impl/unicode.hpp"

#include "ulight/impl/lang/cpp.hpp"
#include "ulight/impl/lang/css.hpp"
#include "ulight/impl/lang/css_chars.hpp"
#include "ulight/impl/lang/html.hpp"

namespace ulight {
namespace css {

bool starts_with_number(std::u8string_view str)
{
    // https://www.w3.org/TR/css-syntax-3/#check-if-three-code-points-would-start-a-number
    if (str.starts_with(u8'+') || str.starts_with(u8'-')) {
        str.remove_prefix(1);
    }
    return !str.empty() && //
        (is_ascii_digit(str[0]) || (str.length() > 1 && str[0] == u8'.' && is_ascii_digit(str[1])));
}

bool starts_with_valid_escape(std::u8string_view str)
{
    // https://www.w3.org/TR/css-syntax-3/#starts-with-a-valid-escape
    return str.length() >= 2 && str[0] == u8'\\' && !is_css_newline(str[1]);
}

bool starts_with_ident_sequence(std::u8string_view str)
{
    // https://www.w3.org/TR/css-syntax-3/#would-start-an-identifier
    if (str.empty()) {
        return false;
    }
    if (str[0] == u8'-') {
        return (str.length() > 1 && is_css_identifier_start(str[1]))
            || starts_with_valid_escape(str.substr(1));
    }
    return is_css_identifier_start(str[0]) || starts_with_valid_escape(str);
}

std::size_t match_number(std::u8string_view str)
{
    // https://www.w3.org/TR/css-syntax-3/#consume-number
    std::size_t length = 0;
    const auto consume_digits = [&] {
        while (length < str.length() && is_ascii_digit(str[length])) {
            ++length;
        }
    };

    if (str.starts_with(u8'+') || str.starts_with(u8'-')) {
        ++length;
    }
    consume_digits();
    if (length + 1 < str.length() && str[length] == u8'.' && is_ascii_digit(str[length + 1])) {
        length += 2;
        consume_digits();
    }
    if (length + 1 < str.length() && (str[length] == u8'e' || str[length] == u8'E')) {
        if ((str[length + 1] == u8'+' || str[length + 1] == u8'-') && length + 2 < str.length()
            && is_ascii_digit(str[length + 2])) {
            length += 3;
        }
        else if (is_ascii_digit(str[length + 2])) {
            length += 2;
        }
        else {
            return length;
        }
        consume_digits();
    }

    return length;
}

std::size_t match_escaped_code_point(std::u8string_view str)
{
    // https://www.w3.org/TR/css-syntax-3/#consume-escaped-code-point
    if (str.empty()) {
        return 0;
    }
    std::size_t length = 0;
    while (is_ascii_hex_digit(str[length]) && length < 6 && length < str.length()) {
        ++length;
    }
    if (length != 0) {
        if (length < str.length() && is_css_whitespace(str[length])) {
            ++length;
        }
        return length;
    }
    return std::size_t(utf8::sequence_length(str[0]));
}

std::size_t match_ident_sequence(std::u8string_view str)
{
    // https://www.w3.org/TR/css-syntax-3/#consume-name
    std::size_t length = 0;
    while (length < str.length()) {
        if (starts_with_valid_escape(str.substr(length))) {
            ++length;
            length += match_escaped_code_point(str.substr(length));
        }
        else if (is_css_identifier(str[length])) {
            ++length;
        }
        else {
            break;
        }
    }
    return length;
}

Ident_Result match_ident_like_token(std::u8string_view str)
{
    // https://www.w3.org/TR/css-syntax-3/#consume-ident-like-token
    // We deviate from the CSS algorithm in the regard that we don't consume the following
    // parenthesis or the following URL, just the identifier.

    const std::size_t length = match_ident_sequence(str);
    const std::u8string_view result = str.substr(length);

    if (length == str.length() || str[length] != u8'(') {
        return { .length = length, .type = Ident_Type::ident };
    }

    if (equals_ascii_ignore_case(result, u8"url")) {
        return { .length = length, .type = Ident_Type::url };
    }

    return { .length = length, .type = Ident_Type::function };
}

namespace {

/// @brief The context that the highlighter is currently in.
/// When highlighting CSS snippets,
/// we cannot be sure what kind of content we're highlighting.
/// For example, we could be highlighting individual values like `"red"`,
/// contents of blocks like `"color: red;"`,
/// or whole stylesheets.
///
/// The `Context` is the best guess we have regarding the content that we're
/// currently highlighting.
enum struct Context : Underlying {
    /// @brief Highlighting top-level content, such as stylesheets.
    top_level,
    /// @brief The prelude of an at rule, such as "@layer something".
    at_prelude,
    /// @brief Highlighting block contents.
    block,
    /// @brief Highlighting individual values.
    value
};

constexpr Highlight_Type selector_highlight_type = Highlight_Type::markup_tag;

struct Highlighter : Highlighter_Base {
private:
    std::size_t brace_level = 0;
    Context context = Context::top_level;

public:
    Highlighter(
        Non_Owning_Buffer<Token>& out,
        std::u8string_view source,
        const Highlight_Options& options
    )
        : Highlighter_Base { out, source, options }
    {
    }

    bool operator()()
    {
        while (!remainder.empty()) {
            consume_comments();
            if (remainder.empty()) {
                break;
            }

            const auto contextual_highlight_type = //
                context == Context::top_level    ? selector_highlight_type
                : context == Context::at_prelude ? Highlight_Type::macro
                : context == Context::block      ? Highlight_Type::markup_attr
                                                 : Highlight_Type::id;

            switch (const char8_t c = remainder.front()) {
            case u8' ':
            case u8'\t':
            case u8'\r':
            case u8'\n':
            case u8'\f': {
                consume_whitespace();
                break;
            }
            case u8'"':
            case u8'\'': {
                consume_string_token(c);
                break;
            }
            case u8'#': {
                if (remainder.length() > 1
                    && (is_css_identifier(remainder[1])
                        || starts_with_valid_escape(remainder.substr(1)))) {
                    const auto highlight_type = context == Context::value
                        ? Highlight_Type::value
                        : contextual_highlight_type;
                    emit_and_advance(1, highlight_type);
                    consume_ident_like_token(highlight_type);
                }
                else {
                    advance(1);
                }
                break;
            }
            case u8'(':
            case u8')': {
                emit_and_advance(1, Highlight_Type::sym_parens);
                break;
            }
            case u8'.': {
                if (starts_with_number(remainder)) {
                    consume_numeric_token();
                }
                else if (context == Context::top_level) {
                    emit_and_advance(1, selector_highlight_type, Coalescing::forced);
                }
                else {
                    advance(1);
                }
                break;
            }
            case u8'+':
            case u8'-': {
                if (starts_with_number(remainder)) {
                    consume_numeric_token();
                    break;
                }
                if (c == u8'-') {
                    const std::u8string_view cdc_token = u8"-->";
                    if (remainder.starts_with(cdc_token)) {
                        emit_and_advance(cdc_token.length(), Highlight_Type::comment_delim);
                        break;
                    }
                    if (starts_with_ident_sequence(remainder.substr(1))) {
                        consume_ident_like_token(Highlight_Type::id);
                        break;
                    }
                }
                emit_and_advance(1, Highlight_Type::error);
                break;
            }
            case u8',': {
                emit_and_advance(1, Highlight_Type::sym_punc);
                break;
            }
            case u8':': {
                switch (context) {
                case Context::top_level: {
                    // Examples: "div:not(.cute)", ":root", "li::before"
                    emit_and_advance(1, selector_highlight_type, Coalescing::forced);
                    break;
                }
                case Context::block: {
                    // Example: "color: red"
                    context = Context::value;
                    emit_and_advance(1, Highlight_Type::sym_punc);
                    break;
                }
                case Context::at_prelude:
                case Context::value: {
                    emit_and_advance(1, Highlight_Type::sym_punc);
                    break;
                }
                }
                break;
            }
            case u8';': {
                if (context == Context::value) {
                    context = Context::block;
                }
                else if (context == Context::at_prelude) {
                    context = Context::top_level;
                }
                emit_and_advance(1, Highlight_Type::sym_punc);
                break;
            }
            case u8'<': {
                constexpr std::u8string_view cdo_token = u8"<!--";
                if (remainder.starts_with(cdo_token)) {
                    emit_and_advance(cdo_token.length(), Highlight_Type::comment_delim);
                }
                else {
                    emit_and_advance(1, Highlight_Type::sym_op);
                }
                break;
            }
            case u8'>':
            case u8'~':
            case u8'*': {
                if (context == Context::top_level) {
                    // Example: "ul > li"
                    emit_and_advance(1, selector_highlight_type, Coalescing::forced);
                }
                else {
                    emit_and_advance(1, Highlight_Type::sym_op);
                }
                break;
            }
            case u8'@': {
                context = Context::at_prelude;
                if (starts_with_ident_sequence(remainder.substr(1))) {
                    emit_and_advance(1, Highlight_Type::macro);
                    consume_ident_like_token(Highlight_Type::macro);
                }
                else {
                    emit_and_advance(1, Highlight_Type::error);
                }
                break;
            }
            case u8'!': {
                // https://www.w3.org/TR/css-syntax-3/#consume-declaration
                constexpr std::u8string_view important = u8"important";
                const std::size_t white_length = html::match_whitespace(remainder.substr(1));
                if (const std::size_t name_length
                    = match_ident_sequence(remainder.substr(1 + white_length))) {
                    if (starts_with_ascii_ignore_case(
                            remainder.substr(1 + white_length, name_length), important
                        )) {
                        emit_and_advance(
                            1 + white_length + important.length(), Highlight_Type::keyword
                        );
                        break;
                    }
                }
                advance(1);
                break;
            }
            case u8'[':
            case u8']': {
                emit_and_advance(1, Highlight_Type::sym_square);
                break;
            }
            case u8'\\': {
                if (starts_with_valid_escape(remainder)) {
                    consume_ident_like_token(contextual_highlight_type);
                }
                else {
                    emit_and_advance(1, Highlight_Type::error);
                }
                break;
            }
            case u8'{': {
                ++brace_level;
                context = Context::block;
                emit_and_advance(1, Highlight_Type::sym_brace);
                break;
            }
            case u8'}': {
                if (brace_level != 0) {
                    --brace_level;
                }
                if (brace_level == 0) {
                    context = Context::top_level;
                }
                emit_and_advance(1, Highlight_Type::sym_brace);
                break;
            }
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
                consume_numeric_token();
                break;
            }
            default: {
                if (is_css_identifier_start(c)) {
                    consume_ident_like_token(contextual_highlight_type);
                }
                else {
                    advance(std::size_t(utf8::sequence_length(c)));
                }
                break;
            }
            }
        }
        return true;
    }

    void consume_whitespace()
    {
        advance(html::match_whitespace(remainder));
    }

    void consume_comments()
    {
        // https://www.w3.org/TR/css-syntax-3/#consume-comment
        while (const cpp::Comment_Result block_comment = cpp::match_block_comment(remainder)) {
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
        }
    }

    void consume_numeric_token()
    {
        // https://www.w3.org/TR/css-syntax-3/#consume-numeric-token
        consume_number();
        if (starts_with_ident_sequence(remainder)) {
            consume_ident_like_token(Highlight_Type::number_decor);
        }
        else if (remainder.starts_with(u8'%')) {
            emit_and_advance(1, Highlight_Type::number_decor);
        }
    }

    void consume_string_token(char8_t quote_char)
    {
        // https://www.w3.org/TR/css-syntax-3/#consume-string-token
        std::size_t length = 1;
        const auto flush = [&] {
            if (length != 0) {
                emit_and_advance(length, Highlight_Type::string);
                length = 0;
            }
        };

        while (length < remainder.length()) {
            if (remainder[length] == quote_char) {
                ++length;
                break;
            }
            if (is_css_newline(remainder[length])) {
                break;
            }
            if (remainder[length] == u8'\\') {
                flush();
                const std::size_t escape_length = match_escaped_code_point(remainder.substr(1)) + 1;
                emit_and_advance(escape_length, Highlight_Type::escape);
                continue;
            }
            ++length;
        }
        flush();
    }

    void consume_number()
    {
        const std::size_t length = match_number(remainder);
        ULIGHT_ASSERT(length != 0);
        emit_and_advance(length, Highlight_Type::number);
    }

    void consume_ident_like_token(Highlight_Type default_type)
    {
        const Ident_Result result = match_ident_like_token(remainder);
        ULIGHT_ASSERT(result);

        const auto actual_type = //
            default_type != Highlight_Type::id    ? default_type
            : result.type == Ident_Type::function ? Highlight_Type::id_function
            : result.type == Ident_Type::url      ? Highlight_Type::keyword
                                                  : Highlight_Type::id;

        std::size_t length = 0;
        const auto flush = [&] {
            if (length != 0) {
                emit_and_advance(length, actual_type, Coalescing::forced);
                length = 0;
            }
        };

        while (length < remainder.length()) {
            if (starts_with_valid_escape(remainder.substr(length))) {
                flush();
                const std::size_t escape_length = match_escaped_code_point(remainder.substr(1)) + 1;
                emit_and_advance(escape_length, Highlight_Type::escape);
            }
            else if (is_css_identifier(remainder[length])) {
                ++length;
            }
            else {
                break;
            }
        }
        flush();
    }
};

} // namespace

} // namespace css

bool highlight_css(
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    std::pmr::memory_resource*,
    const Highlight_Options& options
)
{
    return css::Highlighter { out, source, options }();
}

} // namespace ulight
