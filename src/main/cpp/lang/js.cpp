#include <algorithm>
#include <cstddef>
#include <memory_resource>
#include <optional>
#include <string_view>
#include <vector>

#include "ulight/ulight.hpp"

#include "ulight/impl/assert.hpp"
#include "ulight/impl/buffer.hpp"
#include "ulight/impl/highlight.hpp"
#include "ulight/impl/highlighter.hpp"
#include "ulight/impl/unicode.hpp"
#include "ulight/impl/unicode_algorithm.hpp"

#include "ulight/impl/lang/html.hpp"
#include "ulight/impl/lang/js.hpp"
#include "ulight/impl/lang/js_chars.hpp"

namespace ulight {

namespace js {

#define ULIGHT_JS_TOKEN_TYPE_U8_CODE(id, code, highlight, source) u8##code,
#define ULIGHT_JS_TOKEN_TYPE_LENGTH(id, code, highlight, source) (sizeof(u8##code) - 1),
#define ULIGHT_JS_TOKEN_HIGHLIGHT_TYPE(id, code, highlight, source) (Highlight_Type::highlight),
#define ULIGHT_JS_TOKEN_TYPE_FEATURE_SOURCE(id, code, highlight, source) (Feature_Source::source),

namespace {

inline constexpr std::u8string_view token_type_codes[] {
    ULIGHT_JS_TOKEN_ENUM_DATA(ULIGHT_JS_TOKEN_TYPE_U8_CODE)
};

static_assert(std::ranges::is_sorted(token_type_codes));

inline constexpr unsigned char token_type_lengths[] {
    ULIGHT_JS_TOKEN_ENUM_DATA(ULIGHT_JS_TOKEN_TYPE_LENGTH)
};

inline constexpr Highlight_Type token_type_highlights[] {
    ULIGHT_JS_TOKEN_ENUM_DATA(ULIGHT_JS_TOKEN_HIGHLIGHT_TYPE)
};

inline constexpr Feature_Source token_type_sources[] {
    ULIGHT_JS_TOKEN_ENUM_DATA(ULIGHT_JS_TOKEN_TYPE_FEATURE_SOURCE)
};

} // namespace

/// @brief Returns the in-code representation of `type`.
[[nodiscard]]
std::u8string_view js_token_type_code(Token_Type type) noexcept
{
    return token_type_codes[std::size_t(type)];
}

/// @brief Equivalent to `js_token_type_code(type).length()`.
[[nodiscard]]
std::size_t js_token_type_length(Token_Type type) noexcept
{
    return token_type_lengths[std::size_t(type)];
}

[[nodiscard]]
Highlight_Type js_token_type_highlight(Token_Type type) noexcept
{
    return token_type_highlights[std::size_t(type)];
}

[[nodiscard]]
Feature_Source js_token_type_source(Token_Type type) noexcept
{
    return token_type_sources[std::size_t(type)];
}

[[nodiscard]]
std::optional<Token_Type> js_token_type_by_code(std::u8string_view code) noexcept
{
    const std::u8string_view* const result = std::ranges::lower_bound(token_type_codes, code);
    if (result == std::end(token_type_codes) || *result != code) {
        return {};
    }
    return Token_Type(result - token_type_codes);
}

namespace {

[[nodiscard]]
constexpr bool js_token_type_is_expr_keyword(Token_Type type)
{
    switch (type) {
        using enum Token_Type;
    case kw_return:
    case kw_throw:
    case kw_case:
    case kw_delete:
    case kw_void:
    case kw_typeof:
    case kw_yield:
    case kw_await:
    case kw_instanceof:
    case kw_in:
    case kw_new: return true;
    default: return false;
    }
}

[[nodiscard]]
constexpr bool js_token_type_cannot_precede_regex(Token_Type type)
{
    // TODO: investigate what the proper set here is, or whether it even makes sense for us
    //       to attempt handling these rules properly.
    switch (type) {
        using enum Token_Type;
    case increment:
    case decrement:
    case right_paren:
    case right_bracket:
    case right_brace:
    case plus:
    case minus: return true;
    default: return false;
    }
}

} // namespace

[[nodiscard]]
bool starts_with_line_terminator(std::u8string_view s)
{
    // https://262.ecma-international.org/15.0/index.html#prod-LineTerminator
    return s.starts_with(u8'\n') //
        || s.starts_with(u8'\r') //
        || s.starts_with(u8"\N{LINE SEPARATOR}") //
        || s.starts_with(u8"\N{PARAGRAPH SEPARATOR}");
}

[[nodiscard]]
std::size_t match_line_terminator_sequence(std::u8string_view s)
{
    // https://262.ecma-international.org/15.0/index.html#prod-LineTerminatorSequence
    constexpr std::u8string_view crlf = u8"\r\n";
    constexpr std::u8string_view ls = u8"\N{LINE SEPARATOR}";
    constexpr std::u8string_view ps = u8"\N{PARAGRAPH SEPARATOR}";

    return s.starts_with(u8'\n') ? 1
        : s.starts_with(crlf)    ? crlf.length()
        : s.starts_with(ls)      ? ls.length()
        : s.starts_with(ps)      ? ps.length()
                                 : 0;
}

std::size_t match_whitespace(std::u8string_view str)
{
    // https://262.ecma-international.org/15.0/index.html#sec-white-space
    constexpr auto predicate = [](char32_t c) { return is_js_whitespace(c); };
    const std::size_t result = utf8::find_if_not(str, predicate);
    return result == std::u8string_view::npos ? str.length() : result;
}

std::size_t match_line_comment(std::u8string_view s) noexcept
{
    // https://262.ecma-international.org/15.0/index.html#prod-SingleLineComment
    if (!s.starts_with(u8"//")) {
        return 0;
    }

    std::size_t length = 2;

    while (length < s.length()) {
        if (starts_with_line_terminator(s.substr(length))) {
            return length;
        }
        ++length;
    }

    return length;
}

Comment_Result match_block_comment(std::u8string_view s) noexcept
{
    // https://262.ecma-international.org/15.0/index.html#prod-MultiLineComment
    if (!s.starts_with(u8"/*")) {
        return {};
    }

    std::size_t length = 2; // Skip /*
    while (length < s.length() - 1) { // Find the prefix.
        if (s[length] == u8'*' && s[length + 1] == u8'/') {
            return Comment_Result { .length = length + 2, .is_terminated = true };
        }
        ++length;
    }

    return Comment_Result { .length = s.length(), .is_terminated = false };
}

std::size_t match_hashbang_comment(std::u8string_view s) noexcept
{
    if (!s.starts_with(u8"#!")) {
        return 0;
    }

    std::size_t length = 2;
    while (length < s.length()) {
        if (starts_with_line_terminator(s.substr(length))) {
            return length;
        }
        ++length;
    }

    return length;
}

String_Literal_Result match_string_literal(std::u8string_view str)
{
    // https://262.ecma-international.org/15.0/index.html#sec-literals-string-literals
    if (!str.starts_with(u8'\'') && !str.starts_with(u8'"')) {
        return {};
    }

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
            return String_Literal_Result { .length = length + 1, .terminated = true };
        }
        else if (c == u8'\n') {
            return String_Literal_Result { .length = length, .terminated = false };
        }

        ++length;
    }

    return String_Literal_Result { .length = length, .terminated = false };
}

#if 0
String_Literal_Result match_template(std::u8string_view str)
{
    // https://262.ecma-international.org/15.0/index.html#prod-Template
    if (!str.starts_with(u8'`')) {
        return {};
    }

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
        else if (c == u8'`') {
            return String_Literal_Result { .length = length + 1, .terminated = true };
        }
        else if (c == u8'$' && length + 1 < str.length() && str[length + 1] == u8'{') {
            const std::size_t subst_length = match_template_substitution(str.substr(length));
            if (subst_length == 0) { // Unterminated.
                return String_Literal_Result { .length = str.length(), .terminated = false };
            }
            length += subst_length;
            continue;
        }

        ++length;
    }

    // Unterminated template literal.
    return String_Literal_Result { .length = length, .terminated = false };
}

std::size_t match_template_substitution(std::u8string_view str)
{
    // // https://262.ecma-international.org/15.0/index.html#sec-template-literal-lexical-components
    if (!str.starts_with(u8"${")) {
        return 0;
    }

    std::size_t length = 2;
    int brace_level = 1; // Start with one open brace

    while (length < str.length() && brace_level > 0) {
        const char8_t c = str[length];

        if (c == u8'{') {
            ++brace_level;
        }
        else if (c == u8'}') {
            --brace_level;
        }
        else if (c == u8'"' || c == u8'\'' || c == u8'`') {
            const String_Literal_Result string_result = match_string_literal(str.substr(length));
            if (string_result) {
                length += string_result.length - 1; // -1 because it will be incremented at the end.
            }
        }
        else if (str.substr(length).starts_with(u8"//")) {
            const std::size_t comment_length = match_line_comment(str.substr(length));
            if (comment_length > 0) {
                length += comment_length - 1; // -1 because it will be incremented at the end.
            }
        }
        else if (str.substr(length).starts_with(u8"/*")) {
            const Comment_Result comment_result = match_block_comment(str.substr(length));
            if (comment_result) {
                length
                    += comment_result.length - 1; // -1 because it will be incremented at the end.
            }
        }

        ++length;
    }
    return brace_level == 0 ? length : 0; // the closing brace is found if brace_level is 0.
}
#endif

Digits_Result match_digits(std::u8string_view str, int base)
{
    const auto* const data_end = str.data() + str.length();
    bool erroneous = false;

    char8_t previous = u8'_';
    const auto* const it = std::ranges::find_if_not(str.data(), data_end, [&](char8_t c) {
        if (c == u8'_') {
            erroneous |= previous == u8'_';
            previous = c;
            return true;
        }
        const bool is_digit = is_ascii_digit_base(c, base);
        previous = c;
        return is_digit;
    });
    erroneous |= previous == u8'_';

    const std::size_t length = it == data_end ? str.length() : std::size_t(it - str.data());
    return { .length = length, .erroneous = erroneous };
}

Numeric_Result match_numeric_literal(std::u8string_view str)
{
    if (str.empty()) {
        return {};
    }

    Numeric_Result result {};
    std::size_t length = 0;

    {
        const auto base = //
            str.starts_with(u8"0b") || str.starts_with(u8"0B")   ? 2
            : str.starts_with(u8"0o") || str.starts_with(u8"0O") ? 8
            : str.starts_with(u8"0x") || str.starts_with(u8"0X") ? 16
                                                                 : 10;
        if (base != 10) {
            result.prefix = 2;
            length += result.prefix;
        }
        const auto integer_digits = match_digits(str.substr(result.prefix), base);
        result.integer = integer_digits.length;
        result.erroneous |= integer_digits.erroneous;
        length += result.integer;
    }

    if (str.substr(length).starts_with(u8'.')) {
        result.erroneous |= result.prefix != 0;
        result.fractional = 1;

        const auto [fractional_digits, fractional_error] = match_digits(str.substr(length + 1));
        result.fractional += fractional_digits;
        result.erroneous |= fractional_digits == 0;
        result.erroneous |= fractional_error;

        if (result.prefix == 0 && result.integer == 0 && !is_ascii_digit(str[length + 1])) {
            return {};
        }
        length += result.fractional;
    }

    if (length == 0) {
        return {};
    }

    if (length < str.length() && (str[length] == u8'e' || str[length] == u8'E')) {
        result.exponent = 1;
        result.erroneous |= result.prefix != 0;

        if (length + result.exponent < str.length()
            && (str[length + result.exponent] == u8'+' || str[length + result.exponent] == u8'-')) {
            ++result.exponent;
        }

        const auto [exp_digits, exp_error] = match_digits(str.substr(length + result.exponent));
        result.exponent += exp_digits;
        result.erroneous |= exp_digits == 0;
        result.erroneous |= exp_error;
        length += result.exponent;
    }

    // https://262.ecma-international.org/15.0/index.html#prod-BigIntLiteralSuffix
    if (length < str.length() && str[length] == u8'n') {
        result.suffix = 1;
        result.erroneous |= result.fractional != 0;
        result.erroneous |= result.exponent != 0;
        length += result.suffix;
    }

    result.length = length;
    ULIGHT_DEBUG_ASSERT(
        (result.prefix + result.integer + result.fractional + result.exponent + result.suffix)
        == result.length
    );
    return result;
}

namespace {

[[nodiscard]]
std::size_t match_line_continuation(std::u8string_view str)
{
    // https://262.ecma-international.org/15.0/index.html#prod-LineContinuation
    if (!str.starts_with(u8'\\')) {
        return 0;
    }
    if (const std::size_t terminator = match_line_terminator_sequence(str.substr(1))) {
        return terminator + 1;
    }
    return 0;
}

enum struct Name_Type : Underlying {
    identifier,
    jsx_identifier,
    jsx_attribute_name,
    jsx_element_name
};

std::size_t match_name(std::u8string_view str, Name_Type type)
{
    // https://262.ecma-international.org/15.0/index.html#sec-names-and-keywords
    if (str.empty()) {
        return 0;
    }

    const auto [first_char, first_units] = utf8::decode_and_length_or_throw(str);
    if (!is_js_identifier_start(first_char)) {
        return 0;
    }

    const auto is_part = [&](char32_t c) {
        if (is_js_identifier_part(c)) {
            return true;
        }
        switch (type) {
        case Name_Type::identifier: return false;
        case Name_Type::jsx_identifier: return c == U'-';
        case Name_Type::jsx_attribute_name: return c == U'-' || c == U':';
        case Name_Type::jsx_element_name: return c == U'-' || c == U':' || c == U'.';
        }
        ULIGHT_DEBUG_ASSERT_UNREACHABLE(u8"Invalid Name_Type.");
    };

    return std::size_t(first_units)
        + utf8::length_if(str.substr(std::size_t(first_units)), is_part);
}

[[nodiscard]]
std::size_t match_regex_flags(std::u8string_view str)
{
    // https://262.ecma-international.org/15.0/index.html#prod-RegularExpressionFlags
    constexpr auto predicate = [](char32_t c) { return is_js_identifier_part(c); };
    return utf8::length_if(str, predicate);
}

} // namespace

std::size_t match_identifier(std::u8string_view str)
{
    // https://262.ecma-international.org/15.0/index.html#sec-names-and-keywords
    return match_name(str, Name_Type::identifier);
}

std::size_t match_jsx_identifier(std::u8string_view str)
{
    // https://facebook.github.io/jsx/#prod-JSXIdentifier
    return match_name(str, Name_Type::jsx_identifier);
}

std::size_t match_jsx_element_name(std::u8string_view str)
{
    // https://facebook.github.io/jsx/#prod-JSXElementName
    return match_name(str, Name_Type::jsx_element_name);
}

std::size_t match_jsx_attribute_name(std::u8string_view str)
{
    // https://facebook.github.io/jsx/#prod-JSXAttributeName
    return match_name(str, Name_Type::jsx_attribute_name);
}

std::size_t match_private_identifier(std::u8string_view str)
{
    // https://262.ecma-international.org/15.0/index.html#prod-PrivateIdentifier
    if (str.empty() || str[0] != u8'#') {
        return 0;
    }

    const std::size_t id_length = match_identifier(str.substr(1));
    return id_length == 0 ? 0 : 1 + id_length;
}

namespace {

struct Whitespace_Comment_Consumer {
    virtual void whitespace(std::size_t str) = 0;
    virtual void block_comment(Comment_Result comment) = 0;
    virtual void line_comment(std::size_t comment) = 0;
};

struct Counting_WSC_Consumer : virtual Whitespace_Comment_Consumer {
    std::size_t length = 0;

    void whitespace(std::size_t str) final
    {
        length += str;
    }
    void block_comment(Comment_Result comment) final
    {
        length += comment.length;
    }
    void line_comment(std::size_t comment) final
    {
        length += comment;
    }
};

void match_whitespace_comment_sequence(Whitespace_Comment_Consumer& out, std::u8string_view str)
{
    while (!str.empty()) {
        if (const std::size_t w = match_whitespace(str)) {
            out.whitespace(w);
            str.remove_prefix(w);
            continue;
        }
        if (const Comment_Result b = match_block_comment(str)) {
            out.block_comment(b);
            str.remove_prefix(b.length);
            continue;
        }
        if (const std::size_t l = match_line_comment(str)) {
            out.line_comment(l);
            str.remove_prefix(l);
            continue;
        }
        break;
    }
}

[[nodiscard]]
std::size_t match_whitespace_comment_sequence(std::u8string_view str)
{
    Counting_WSC_Consumer out;
    match_whitespace_comment_sequence(out, str);
    return out.length;
}

} // namespace

[[nodiscard]]
JSX_Braced_Result match_jsx_braced(std::u8string_view str)
{
    // https://facebook.github.io/jsx/#prod-JSXSpreadAttribute
    if (!str.starts_with(u8'{')) {
        return {};
    }
    std::size_t length = 1;
    std::size_t level = 1;

    while (length < str.length()) {
        if (const std::size_t skip_length = match_whitespace_comment_sequence(str.substr(length))) {
            length += skip_length;
        }
        switch (str[length]) {
        case u8'{': {
            ++level;
            ++length;
            break;
        }
        case u8'}': {
            ++length;
            if (--level == 0) {
                return { .length = length, .is_terminated = true };
            }
            break;
        }
        case u8'\'':
        case u8'"': {
            const String_Literal_Result s = match_string_literal(str.substr(length));
            length += s ? s.length : 1;
            break;
        }
        default: {
            ++length;
        }
        }
    }
    return { .length = length, .is_terminated = false };
}

namespace {

struct JSX_Tag_Consumer : virtual Whitespace_Comment_Consumer {
    virtual void done(JSX_Type) = 0;
    virtual void advance(std::size_t amount) = 0;
    virtual void opening_symbol() = 0;
    virtual void element_name(std::size_t name) = 0;
    virtual void closing_symbol() = 0;
    virtual void attribute_name(std::size_t name) = 0;
    virtual void attribute_equals() = 0;
    virtual void string_literal(String_Literal_Result r) = 0;
    virtual void braced(JSX_Braced_Result braced) = 0;
};

struct Counting_JSX_Tag_Consumer final : JSX_Tag_Consumer, Counting_WSC_Consumer {
    JSX_Type type {};

    void done(JSX_Type t) final
    {
        type = t;
    }
    void advance(std::size_t amount) final
    {
        length += amount;
    }
    void opening_symbol() final
    {
        ++length;
    }
    void element_name(std::size_t name) final
    {
        length += name;
    }
    void attribute_name(std::size_t name) final
    {
        length += name;
    }
    void closing_symbol() final
    {
        ++length;
    }
    void attribute_equals() final
    {
        ++length;
    }
    void string_literal(String_Literal_Result r) final
    {
        length += r.length;
    }
    void braced(JSX_Braced_Result braced) final
    {
        length += braced.length;
    }
};

struct Matching_JSX_Tag_Consumer final : JSX_Tag_Consumer {
    JSX_Tag_Consumer& out;
    std::u8string_view& str;

    Matching_JSX_Tag_Consumer(JSX_Tag_Consumer& out, std::u8string_view& str)
        : out { out }
        , str { str }
    {
    }

    void done(JSX_Type type) final
    {
        out.done(type);
    }
    void whitespace(std::size_t w) final
    {
        out.whitespace(w);
        str.remove_prefix(w);
    }
    void block_comment(Comment_Result comment) final
    {
        out.block_comment(comment);
        str.remove_prefix(comment.length);
    }
    void line_comment(std::size_t l) final
    {
        out.line_comment(l);
        str.remove_prefix(l);
    }
    void advance(std::size_t amount) final
    {
        out.advance(amount);
        str.remove_prefix(amount);
    }
    void opening_symbol() final
    {
        out.opening_symbol();
        str.remove_prefix(1);
    }
    void closing_symbol() final
    {
        out.closing_symbol();
        str.remove_prefix(1);
    }
    void element_name(std::size_t name) final
    {
        out.element_name(name);
        str.remove_prefix(name);
    }
    void attribute_name(std::size_t name) final
    {
        out.attribute_name(name);
        str.remove_prefix(name);
    }
    void attribute_equals() final
    {
        out.attribute_equals();
        str.remove_prefix(1);
    }
    void string_literal(String_Literal_Result r) final
    {
        out.string_literal(r);
        str.remove_prefix(r.length);
    }
    void braced(JSX_Braced_Result braced) final
    {
        out.braced(braced);
        str.remove_prefix(braced.length);
    }
};

enum struct JSX_Tag_Subset : bool {
    all,
    non_closing,
};

bool match_jsx_tag_impl(
    JSX_Tag_Consumer& consumer,
    std::u8string_view str,
    JSX_Tag_Subset subset = JSX_Tag_Subset::all
)
{
    // https://facebook.github.io/jsx/#prod-JSXElement
    // https://facebook.github.io/jsx/#prod-JSXFragment
    if (!str.starts_with(u8'<')) {
        return {};
    }

    Matching_JSX_Tag_Consumer out { consumer, str };

    out.opening_symbol();
    match_whitespace_comment_sequence(out, str);

    if (str.starts_with(u8'>')) {
        out.closing_symbol();
        out.done(JSX_Type::fragment_opening);
        return true;
    }
    bool closing = false;
    if (str.starts_with(u8'/')) {
        if (subset == JSX_Tag_Subset::non_closing) {
            return false;
        }
        closing = true;
        out.closing_symbol();
        match_whitespace_comment_sequence(out, str);
        if (str.starts_with(u8'>')) {
            out.closing_symbol();
            out.done(JSX_Type::fragment_closing);
            return true;
        }
    }
    if (const std::size_t id_length = match_jsx_element_name(str)) {
        out.element_name(id_length);
    }

    while (!str.empty()) {
        match_whitespace_comment_sequence(out, str);
        if (str.starts_with(u8'>')) {
            out.closing_symbol();
            out.done(closing ? JSX_Type::closing : JSX_Type::opening);
            return true;
        }
        if (str.starts_with(u8"/>")) {
            if (closing) {
                return false;
            }
            out.closing_symbol();
            out.closing_symbol();
            out.done(JSX_Type::self_closing);
            return true;
        }
        // https://facebook.github.io/jsx/#prod-JSXAttributes
        if (const JSX_Braced_Result spread = match_jsx_braced(str)) {
            if (!spread.is_terminated) {
                return false;
            }
            out.braced(spread);
            continue;
        }
        if (const std::size_t attr_name_length = match_jsx_attribute_name(str)) {
            // https://facebook.github.io/jsx/#prod-JSXAttributes
            out.attribute_name(attr_name_length);
            match_whitespace_comment_sequence(out, str);
            if (!str.starts_with(u8'=')) {
                continue;
            }
            out.attribute_equals();
            match_whitespace_comment_sequence(out, str);
            // https://facebook.github.io/jsx/#prod-JSXAttributeValue
            if (const String_Literal_Result s = match_string_literal(str)) {
                out.string_literal(s);
                continue;
            }
            if (const JSX_Braced_Result b = match_jsx_braced(str)) {
                if (!b.is_terminated) {
                    return false;
                }
                out.braced(b);
                continue;
            }
            // Technically, JSX allows for elements and fragments to appear as
            // attribute values.
            // However, this would require recursive parsing at this point,
            // and we currently don't support it.
            //
            // It looks like other highlighters such as the VSCode highlighter also
            // don't support this behavior.
        }
        break;
    }

    return false;
}

[[nodiscard]]
JSX_Tag_Result
match_jsx_tag_impl(std::u8string_view str, JSX_Tag_Subset subset = JSX_Tag_Subset::all)
{
    Counting_JSX_Tag_Consumer out;
    if (match_jsx_tag_impl(out, str, subset)) {
        return { out.length, out.type };
    }
    return {};
}

} // namespace

JSX_Tag_Result match_jsx_tag(std::u8string_view str)
{
    return match_jsx_tag_impl(str);
}

namespace {

std::optional<Token_Type> match_operator_or_punctuation(std::u8string_view str)
{
    using enum Token_Type;

    if (str.empty()) {
        return {};
    }

    switch (str[0]) {
    case u8'!':
        return str.starts_with(u8"!==") ? strict_not_equals
            : str.starts_with(u8"!=")   ? not_equals
                                        : logical_not;

    case u8'%': return str.starts_with(u8"%=") ? modulo_equal : modulo;

    case u8'&':
        return str.starts_with(u8"&&=") ? logical_and_equal
            : str.starts_with(u8"&&")   ? logical_and
            : str.starts_with(u8"&=")   ? bitwise_and_equal
                                        : bitwise_and;

    case u8'(': return left_paren;
    case u8')': return right_paren;

    case u8'*':
        return str.starts_with(u8"**=") ? exponentiation_equal
            : str.starts_with(u8"**")   ? exponentiation
            : str.starts_with(u8"*=")   ? multiply_equal
                                        : multiply;

    case u8'+':
        return str.starts_with(u8"++") ? increment : str.starts_with(u8"+=") ? plus_equal : plus;

    case u8',': return comma;

    case u8'-':
        return str.starts_with(u8"--") ? decrement : str.starts_with(u8"-=") ? minus_equal : minus;

    case u8'.': return str.starts_with(u8"...") ? ellipsis : dot;

    case u8'/': return str.starts_with(u8"/=") ? divide_equal : divide;

    case u8':': return colon;
    case u8';': return semicolon;

    case u8'<': {
        return str.starts_with(u8"<<=") ? left_shift_equal
            : str.starts_with(u8"<<")   ? left_shift
            : str.starts_with(u8"<=")   ? less_equal
                                        : less_than;
    }

    case u8'=': {
        return str.starts_with(u8"===") ? strict_equals
            : str.starts_with(u8"==")   ? equals
            : str.starts_with(u8"=>")   ? arrow
                                        : assignment;
    }

    case u8'>': {
        return str.starts_with(u8">>>=") ? unsigned_right_shift_equal
            : str.starts_with(u8">>>")   ? unsigned_right_shift
            : str.starts_with(u8">>=")   ? right_shift_equal
            : str.starts_with(u8">>")    ? right_shift
            : str.starts_with(u8">=")    ? greater_equal
                                         : greater_than;
    }

    case u8'?':
        return str.starts_with(u8"??=") ? nullish_coalescing_equal
            : str.starts_with(u8"??")   ? nullish_coalescing
            : str.starts_with(u8"?.")   ? optional_chaining
                                        : conditional;

    case u8'[': return left_bracket;
    case u8']': return right_bracket;

    case u8'^': return str.starts_with(u8"^=") ? bitwise_xor_equal : bitwise_xor;

    case u8'{': return left_brace;

    case u8'|':
        return str.starts_with(u8"||=") ? logical_or_equal
            : str.starts_with(u8"||")   ? logical_or
            : str.starts_with(u8"|=")   ? bitwise_or_equal
                                        : bitwise_or;

    case u8'}': return right_brace;

    case u8'~': return bitwise_not;

    default: return {};
    }
}

} // namespace

namespace {

/// @brief The JS tokenizer is context-sensitive.
/// A lot of that has to do with avoiding recursion when parsing the contents of template literals,
/// and some of it has to do with allowing RegularExpressionLiteral and Hashbang only in
/// some contexts.
///
/// `hashbang_or_regex` is only used at the start of the file.
/// From that point on, the decision is based on whether a regex literal can appear in the
/// context-free grammar.
enum struct Input_Element : Underlying {
    // https://262.ecma-international.org/15.0/index.html#prod-InputElementHashbangOrRegExp
    hashbang_or_regex,
    // https://262.ecma-international.org/15.0/index.html#prod-InputElementRegExp
    regex,
    // https://262.ecma-international.org/15.0/index.html#prod-InputElementDiv
    div,
};

[[nodiscard]]
constexpr bool input_element_has_hashbang(Input_Element goal)
{
    return goal == Input_Element::hashbang_or_regex;
}

[[nodiscard]]
constexpr bool input_element_has_regex(Input_Element goal)
{
    return goal == Input_Element::hashbang_or_regex || goal == Input_Element::regex;
}

/// @brief  Common JS and JSX highlighter implementation.
struct [[nodiscard]] Highlighter : Highlighter_Base {
private:
    Input_Element input_element = Input_Element::hashbang_or_regex;

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
            consume_token();
        }
        return true;
    }

private:
    /// @brief Consumes braced JS code.
    /// This is used both for matching braced JS code in JSX, like in `<div id={get_id()}>`,
    /// and for template literals in regular JS.
    ///
    /// The closing brace is not consumed.
    void consume_js_before_closing_brace()
    {
        input_element = Input_Element::regex;
        int brace_level = 0;
        while (!remainder.empty()) {
            if (remainder[0] == u8'{') {
                ++brace_level;
                emit_and_advance(1, Highlight_Type::sym_brace);
                input_element = Input_Element::regex;
                continue;
            }
            if (remainder[0] == u8'}') {
                if (--brace_level < 0) {
                    return;
                }
                emit_and_advance(1, Highlight_Type::sym_brace);
                input_element = Input_Element::div;
                continue;
            }

            consume_token();
        }
    }

    void consume_token()
    {
        if (expect_whitespace() || //
            expect_hashbang_comment() || //
            expect_line_comment() || //
            expect_block_comment() || //
            expect_jsx_in_js() || //
            expect_string_literal() || //
            expect_template() || //
            expect_regex() || //
            expect_numeric_literal() || //
            expect_private_identifier() || //
            expect_symbols() || //
            expect_operator_or_punctuation()) {
            return;
        }
        consume_error();
    }

    void consume_error()
    {
        emit_and_advance(1, Highlight_Type::error);
        input_element = Input_Element::regex;
    }

    bool expect_jsx_in_js()
    {
        // JSX parsing is a bit insane.
        // In short, we first trial-parse some JSX tag, say, "<div class='abc'>".
        // This requires arbitrary lookahead.
        // Only once we've successfully parsed a tag, we consider it to be a JSX tag.
        // Otherwise, we fall back onto regular JS semantics,
        // and consider "<" to be the less-than operator instead.
        //
        // Furthermore, we ignore closing tags at the beginning.

        const JSX_Tag_Result opening = match_jsx_tag_impl(remainder, JSX_Tag_Subset::non_closing);
        if (!opening) {
            return false;
        }
        consume_jsx_tag();
        if (opening.type != JSX_Type::self_closing) {
            const bool is_opening
                = opening.type == JSX_Type::opening || opening.type == JSX_Type::fragment_opening;
            ULIGHT_ASSERT(is_opening);
            consume_jsx_children_and_closing_tag();
        }
        input_element = Input_Element::div;
        return true;
    }

    void consume_jsx_tag()
    {
        struct Highlighter_As_Consumer : JSX_Tag_Consumer {
            Highlighter& self;
            Highlighter_As_Consumer(Highlighter& self)
                : self { self }
            {
            }

            void done(JSX_Type) final { }
            void whitespace(std::size_t w) final
            {
                self.advance(w);
            }
            void block_comment(Comment_Result comment) final
            {
                self.highlight_block_comment(comment);
            }
            void line_comment(std::size_t l) final
            {
                self.highlight_line_comment(l);
            }
            void advance(std::size_t amount) final
            {
                self.advance(amount);
            }
            void opening_symbol() final
            {
                self.emit_and_advance(1, Highlight_Type::sym_punc);
            }
            void closing_symbol() final
            {
                self.emit_and_advance(1, Highlight_Type::sym_punc);
            }
            void element_name(std::size_t name) final
            {
                self.emit_and_advance(name, Highlight_Type::markup_tag);
            }
            void attribute_name(std::size_t name) final
            {
                self.emit_and_advance(name, Highlight_Type::markup_tag);
            }
            void attribute_equals() final
            {
                self.emit_and_advance(1, Highlight_Type::sym_punc);
            }
            void string_literal(String_Literal_Result r) final
            {
                self.highlight_string_literal(r);
            }
            void braced(JSX_Braced_Result braced) final
            {
                ULIGHT_ASSERT(braced.is_terminated && braced.length >= 2);
                self.highlight_jsx_braced(braced);
            }

        } out { *this };

        match_jsx_tag_impl(out, remainder);
    }

    void consume_jsx_children_and_closing_tag()
    {
        // https://facebook.github.io/jsx/#prod-JSXChildren
        int depth = 0;
        std::u8string_view rem = remainder;
        while (!rem.empty()) {
            // https://facebook.github.io/jsx/#prod-JSXText
            const std::size_t safe_length = rem.find_first_of(u8"&{}<>");
            if (safe_length == std::u8string_view::npos) {
                advance(rem.length());
                break;
            }
            advance(safe_length);
            rem.remove_prefix(safe_length);

            switch (rem[0]) {
            case u8'&': {
                // https://facebook.github.io/jsx/#prod-HTMLCharacterReference
                if (const std::size_t ref = html::match_character_reference(rem)) {
                    emit_and_advance(ref, Highlight_Type::escape);
                    rem.remove_prefix(ref);
                }
                else {
                    advance(1);
                    rem.remove_prefix(1);
                }
                continue;
            }
            case u8'<': {
                // https://facebook.github.io/jsx/#prod-JSXElement
                const JSX_Tag_Result tag = match_jsx_tag(rem);
                if (!tag) {
                    emit_and_advance(1, Highlight_Type::error);
                    rem.remove_prefix(1);
                    continue;
                }
                consume_jsx_tag();
                rem.remove_prefix(tag.length);
                if (tag.type == JSX_Type::opening || tag.type == JSX_Type::fragment_opening) {
                    ++depth;
                }
                if (tag.type == JSX_Type::closing || tag.type == JSX_Type::fragment_closing) {
                    if (--depth < 0) {
                        return;
                    }
                }
                continue;
            }
            case u8'>': {
                // Stray ">".
                // This should have been part of a tag.
                emit_and_advance(1, Highlight_Type::error);
                rem.remove_prefix(1);
                continue;
            }
            case u8'{': {
                // https://facebook.github.io/jsx/#prod-JSXChild
                const JSX_Braced_Result braced = match_jsx_braced(rem);
                if (braced) {
                    highlight_jsx_braced(braced);
                    rem.remove_prefix(braced.length);
                }
                else {
                    emit_and_advance(1, Highlight_Type::error);
                    rem.remove_prefix(1);
                }
                continue;
            }
            case u8'}': {
                // Stray "}".
                // This should have been part of a braced child expression.
                emit_and_advance(1, Highlight_Type::error);
                rem.remove_prefix(1);
                continue;
            }
            default: ULIGHT_ASSERT_UNREACHABLE();
            }
        }
        // Unterminated JSX child content.
        // This isn't really valid code, but it doesn't matter for syntax highlighting.
    }

    void highlight_jsx_braced(const JSX_Braced_Result& braced)
    {
        ULIGHT_ASSERT(braced);
        ULIGHT_ASSERT(remainder.starts_with(u8'{'));

        emit_and_advance(1, Highlight_Type::sym_brace);
        const std::size_t js_length = braced.length - (braced.is_terminated ? 2 : 1);

        if (js_length != 0) {
            consume_js_before_closing_brace();
        }
        if (braced.is_terminated) {
            emit_and_advance(1, Highlight_Type::sym_brace);
        }
    }

    bool expect_whitespace()
    {
        const std::size_t white_length = match_whitespace(remainder);
        advance(white_length);
        return white_length != 0;
    }

    bool expect_hashbang_comment()
    {
        // https://262.ecma-international.org/15.0/index.html#sec-hashbang
        if (!input_element_has_hashbang(input_element)) {
            return false;
        }

        const std::size_t hashbang_length = match_hashbang_comment(remainder);
        if (hashbang_length == 0) {
            return false;
        }

        emit_and_advance(2, Highlight_Type::comment_delim); // #!
        if (hashbang_length > 2) {
            emit_and_advance(hashbang_length - 2, Highlight_Type::comment);
        }
        return true;
    }

    bool expect_line_comment()
    {
        // https://262.ecma-international.org/15.0/index.html#prod-SingleLineComment
        if (const std::size_t length = match_line_comment(remainder)) {
            highlight_line_comment(length);
            return true;
        }
        return false;
    }

    void highlight_line_comment(std::size_t length)
    {
        emit_and_advance(2, Highlight_Type::comment_delim); // //
        if (length > 2) {
            emit_and_advance(length - 2, Highlight_Type::comment);
        }
        input_element = Input_Element::regex;
    }

    bool expect_block_comment()
    {
        // https://262.ecma-international.org/15.0/index.html#prod-MultiLineComment
        if (const Comment_Result block_comment = match_block_comment(remainder)) {
            highlight_block_comment(block_comment);
            return true;
        }
        return false;
    }

    void highlight_block_comment(const Comment_Result& block_comment)
    {
        ULIGHT_DEBUG_ASSERT(block_comment);
        emit(index, 2, Highlight_Type::comment_delim); // /*
        const std::size_t suffix_length = block_comment.is_terminated ? 2 : 0;
        const std::size_t content_length = block_comment.length - 2 - suffix_length;
        if (content_length != 0) {
            emit(index + 2, content_length, Highlight_Type::comment);
        }
        if (block_comment.is_terminated) {
            emit(index + block_comment.length - 2, 2, Highlight_Type::comment_delim); // */
        }
        advance(block_comment.length);
        input_element = Input_Element::regex;
    }

    bool expect_string_literal()
    {
        if (const String_Literal_Result string = match_string_literal(remainder)) {
            highlight_string_literal(string);
            return true;
        }
        return false;
    }

    void highlight_string_literal(const String_Literal_Result& string)
    {
        ULIGHT_ASSERT(string);
        emit_and_advance(1, Highlight_Type::string_delim);
        const std::size_t content_length = string.length - (string.terminated ? 2 : 1);
        if (content_length != 0) {
            emit_and_advance(content_length, Highlight_Type::string);
        }
        if (string.terminated) {
            emit_and_advance(1, Highlight_Type::string_delim);
        }
        input_element = Input_Element::div;
    }

    bool expect_template()
    {
        // https://262.ecma-international.org/15.0/index.html#sec-template-literal-lexical-components
        if (remainder.starts_with(u8'`')) {
            consume_template();
            return true;
        }
        return false;
    }

    void consume_template()
    {
        // https://262.ecma-international.org/15.0/index.html#sec-template-literal-lexical-components
        ULIGHT_ASSERT(remainder.starts_with(u8'`'));
        emit_and_advance(1, Highlight_Type::string_delim);

        std::size_t chars = 0;
        const auto flush_chars = [&] {
            if (chars != 0) {
                emit(index - chars, chars, Highlight_Type::string);
                chars = 0;
            }
        };

        while (!remainder.empty()) {
            const std::u8string_view rem = remainder;

            switch (rem[0]) {
            case u8'`': {
                flush_chars();
                emit_and_advance(1, Highlight_Type::string_delim);
                input_element = Input_Element::div;
                return;
            }
            case u8'$': {
                if (rem.starts_with(u8"${")) {
                    flush_chars();
                    emit_and_advance(2, Highlight_Type::escape);
                    consume_js_before_closing_brace();
                    if (!remainder.empty()) {
                        ULIGHT_ASSERT(remainder.starts_with(u8'}'));
                        emit_and_advance(1, Highlight_Type::escape);
                    }
                    // Otherwise, we have an unterminated substitution.
                    continue;
                }
                advance(1);
                ++chars;
                continue;
            }
            case u8'\\': {
                if (const std::size_t c = match_line_continuation(rem)) {
                    ULIGHT_ASSERT(c > 1);
                    flush_chars();
                    emit_and_advance(1, Highlight_Type::escape);
                    advance(c - 1);
                    chars += c - 1;
                    continue;
                }
                // TODO: there are other escape sequence, and these should be highlighted
                advance(1);
                ++chars;
                continue;
            }
            default: {
                advance(1);
                ++chars;
                continue;
            }
            }
        }

        flush_chars();
        // Unterminated template.
    }

    bool expect_regex()
    {
        // https://262.ecma-international.org/15.0/index.html#sec-literals-regular-expression-literals
        if (!input_element_has_regex(input_element)) {
            return false;
        }

        std::u8string_view rem = remainder;

        if (!rem.starts_with(u8'/') || rem.starts_with(u8"/*") || rem.starts_with(u8"//")) {
            return false;
        }

        rem.remove_prefix(1);

        auto escaped = false;

        for (std::size_t size = 0; size < rem.length(); ++size) {
            const char8_t c = rem[size];

            if (escaped) {
                escaped = false;
            }
            else if (c == u8'\\') {
                escaped = true;
            }
            else if (c == u8'/') {
                emit_and_advance(1, Highlight_Type::string_delim);
                emit_and_advance(size, Highlight_Type::string);
                emit_and_advance(1, Highlight_Type::string_delim);
                if (const std::size_t regex_flags = match_regex_flags(rem.substr(size + 1))) {
                    emit_and_advance(regex_flags, Highlight_Type::string_decor);
                }
                input_element = Input_Element::div;
                return true;
            }
            else if (starts_with_line_terminator(rem.substr(size))) {
                break;
            }
        }

        return false;
    }

    bool expect_numeric_literal()
    {
        const Numeric_Result number = match_numeric_literal(remainder);
        if (!number) {
            return false;
        }
        if (number.erroneous) {
            emit_and_advance(number.length, Highlight_Type::error);
        }
        else {
            // TODO: more granular output
            emit_and_advance(number.length, Highlight_Type::number);
        }
        input_element = Input_Element::div;
        return true;
    }

    bool expect_private_identifier()
    {
        if (const std::size_t private_id_length = match_private_identifier(remainder)) {
            emit_and_advance(private_id_length, Highlight_Type::id);
            input_element = Input_Element::div;
            return true;
        }
        return false;
    }

    bool expect_symbols()
    {
        const std::size_t id_length = match_identifier(remainder);
        if (id_length == 0) {
            return false;
        }

        const std::optional<Token_Type> keyword
            = js_token_type_by_code(remainder.substr(0, id_length));
        if (!keyword) {
            emit_and_advance(id_length, Highlight_Type::id);
            input_element = Input_Element::div;
            return true;
        }

        const Highlight_Type highlight = js_token_type_highlight(*keyword);
        emit_and_advance(id_length, highlight);

        input_element
            = js_token_type_is_expr_keyword(*keyword) ? Input_Element::regex : Input_Element::div;

        return true;
    }

    bool expect_operator_or_punctuation()
    {
        const std::optional<Token_Type> op = match_operator_or_punctuation(remainder);
        if (!op) {
            return false;
        }
        const std::size_t op_length = js_token_type_length(*op);
        const Highlight_Type op_highlight = js_token_type_highlight(*op);

        emit_and_advance(op_length, op_highlight);
        input_element
            = js_token_type_cannot_precede_regex(*op) ? Input_Element::div : Input_Element::regex;

        return true;
    }
};

} // namespace

} // namespace js

bool highlight_javascript(
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    std::pmr::memory_resource*,
    const Highlight_Options& options
)
{
    return js::Highlighter { out, source, options }();
}

} // namespace ulight
