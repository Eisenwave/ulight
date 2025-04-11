#include <algorithm>
#include <cstddef>
#include <memory_resource>
#include <string_view>

#include "ulight/ulight.hpp"

#include "ulight/impl/assert.hpp"
#include "ulight/impl/buffer.hpp"
#include "ulight/impl/chars.hpp"
#include "ulight/impl/highlight.hpp"
#include "ulight/impl/html.hpp"
#include "ulight/impl/strings.hpp"
#include "ulight/impl/unicode_algorithm.hpp"

namespace ulight {
namespace html {

namespace {

constexpr std::u8string_view comment_prefix = u8"<!--";
constexpr std::u8string_view comment_suffix = u8"-->";
constexpr std::u8string_view comment_suffix_degenerate = u8"<!-->";

constexpr std::u8string_view cdata_prefix = u8"<![CDATA[";
constexpr std::u8string_view cdata_suffix = u8"]]>";

constexpr std::u8string_view doctype_prefix = u8"<!DOCTYPE";

bool is_character_reference_content(std::u8string_view str)
{
    if (str.starts_with(u8"#x")) {
        return str.length() > 2
            && std::ranges::all_of(str.substr(2), [](char8_t c) { return is_ascii_hex_digit(c); });
    }
    if (str.starts_with(u8'#')) {
        return str.length() > 1
            && std::ranges::all_of(str.substr(1), [](char8_t c) { return is_ascii_digit(c); });
    }
    return !str.empty()
        && std::ranges::all_of(str, [](char8_t c) { return is_ascii_alphanumeric(c); });
}

} // namespace

std::size_t match_whitespace(std::u8string_view str)
{
    constexpr auto predicate = [](char8_t c) { return is_html_whitespace(c); };
    return std::size_t(std::ranges::find_if_not(str, predicate) - str.begin());
}

std::size_t match_character_reference(std::u8string_view str)
{
    if (!str.starts_with(u8'&')) {
        return 0;
    }
    const std::size_t result = str.find(u8';', 1);
    const bool success = result != std::u8string_view::npos
        && is_character_reference_content(str.substr(1, result - 1));
    return success ? result + 1 : 0;
}

std::size_t match_tag_name(std::u8string_view str)
{
    const std::size_t result = utf8::find_if_not(str, [](char32_t c) { //
        return is_html_tag_name_character(c);
    });
    return result == std::u8string_view::npos ? str.length() : result;
}

std::size_t match_attribute_name(std::u8string_view str)
{
    const std::size_t result = utf8::find_if_not(str, [](char32_t c) { //
        return is_html_attribute_name_character(c);
    });
    return result == std::u8string_view::npos ? str.length() : result;
}

std::size_t match_raw_text(std::u8string_view str, std::u8string_view closing_name)
{
    // https://html.spec.whatwg.org/dev/syntax.html#cdata-rcdata-restrictions
    std::size_t length = 0;

    while (!str.empty()) {
        const std::size_t safe_length = str.find(u8"</");
        if (safe_length == std::u8string_view::npos) {
            return length + str.length();
        }
        length += safe_length;
        str.remove_prefix(safe_length);
        // At this point, there are three parts that can terminate raw text:
        //   1. "</", which we've already matched.
        //   2. The closing name.
        //   3. Whitespace, ">", or "/".
        //
        // We're not sure if we'll get all three, so we use new_length as a temporary length
        // and only commit to updating length once all parts are matched.
        std::size_t new_length = length + 2;
        str.remove_prefix(2);
        if (!starts_with_ascii_ignore_case(str, closing_name)) {
            length = new_length;
            continue;
        }
        new_length += closing_name.length();
        str.remove_prefix(closing_name.length());
        if (str.empty()) {
            return new_length;
        }
        const char8_t c = str.front();
        if (is_html_whitespace(c) || c == u8'>' || c == u8'/') {
            return length;
        }
        length += 2;
        str.remove_prefix(2);
    }
    return length;
}

Raw_Text_Result
match_escapable_raw_text_piece(std::u8string_view str, std::u8string_view closing_name)
{
    // https://html.spec.whatwg.org/dev/syntax.html#cdata-rcdata-restrictions
    std::size_t length = 0;

    while (!str.empty()) {
        const std::size_t safe_length = str.find_first_of(u8"<&");
        if (safe_length == std::u8string_view::npos) {
            return { .raw_length = length + str.length(), .ref_length = 0 };
        }
        length += safe_length;
        str.remove_prefix(safe_length);
        if (const std::size_t ref_length = match_character_reference(str)) {
            return { .raw_length = length, .ref_length = ref_length };
        }
        if (!str.starts_with(u8"</")) {
            ++length;
            str.remove_prefix(1);
            continue;
        }
        // At this point, there are three parts that can terminate raw text:
        //   1. "</", which we've already matched.
        //   2. The closing name.
        //   3. Whitespace, ">", or "/".
        //
        // We're not sure if we'll get all three, so we use new_length as a temporary length
        // and only commit to updating length once all parts are matched.
        std::size_t new_length = length + 2;
        str.remove_prefix(2);
        if (!starts_with_ascii_ignore_case(str, closing_name)) {
            length = new_length;
            continue;
        }
        new_length += closing_name.length();
        str.remove_prefix(closing_name.length());
        if (str.empty()) {
            return { .raw_length = new_length, .ref_length = 0 };
        }
        const char8_t c = str.front();
        if (is_html_whitespace(c) || c == u8'>' || c == u8'/') {
            return { .raw_length = length, .ref_length = 0 };
        }
    }
    return { .raw_length = length, .ref_length = 0 };
}

Match_Result match_comment(std::u8string_view str)
{
    // https://html.spec.whatwg.org/dev/syntax.html#comments

    if (!str.starts_with(comment_prefix)) {
        return {};
    }

    std::size_t length = comment_prefix.length();
    str.remove_prefix(comment_prefix.length());
    if (str.starts_with(u8'>') || str.starts_with(u8"->")) {
        return {};
    }

    while (!str.empty()) {
        const std::size_t plain_prefix_length = str.find_first_of(u8"<-");
        if (plain_prefix_length == std::u8string_view::npos) {
            return { .length = length + str.length(), .terminated = false };
        }
        str.remove_prefix(plain_prefix_length);
        length += plain_prefix_length;

        if (str.starts_with(comment_suffix)) {
            return { .length = length + comment_suffix.length(), .terminated = true };
        }
        if (str.starts_with(u8"<!--")) {
            if (str.starts_with(comment_suffix_degenerate)) {
                return { .length = length + comment_suffix_degenerate.length(),
                         .terminated = true };
            }
            return {};
        }
        if (str.starts_with(u8"--!>")) {
            return {};
        }
        ++length;
        str.remove_prefix(1);
    }

    return { .length = length, .terminated = false };
}

Match_Result match_doctype_permissive(std::u8string_view str)
{
    // https://html.spec.whatwg.org/dev/syntax.html#the-doctype
    if (!str.starts_with(doctype_prefix)) {
        return {};
    }
    const std::size_t result = str.find(u8'>', doctype_prefix.length());
    if (result == std::u8string_view::npos) {
        return { .length = str.length(), .terminated = false };
    }
    return { .length = result + 1, .terminated = true };
}

Match_Result match_cdata(std::u8string_view str)
{
    // https://html.spec.whatwg.org/dev/syntax.html#syntax-cdata
    if (!str.starts_with(cdata_prefix)) {
        return {};
    }
    const std::size_t result = str.find(cdata_suffix, cdata_prefix.length());
    if (result == std::u8string_view::npos) {
        return { .length = str.length(), .terminated = false };
    }
    return { .length = result + cdata_suffix.length(), .terminated = true };
}

End_Tag_Result match_end_tag_permissive(std::u8string_view str)
{
    // https://html.spec.whatwg.org/dev/syntax.html#end-tags
    if (!str.starts_with(u8"</")) {
        return {};
    }

    const auto* const end_pointer = str.data() + str.size();
    const auto* const pos = std::ranges::find_if(str.data() + 2, end_pointer, [](char8_t c) static {
        return is_html_whitespace(c) || c == u8'>';
    });
    if (pos == end_pointer) {
        return {};
    }
    const auto name_length = std::size_t(pos - str.data() - 2);
    if (name_length == 0) {
        return {};
    }

    if (*pos == u8'>') {
        return { .length = std::size_t(pos - str.data()) + 1, .name_length = name_length };
    }

    const auto* const pos2 = std::ranges::find(pos, end_pointer, u8'>');
    if (pos2 == end_pointer) {
        return {};
    }

    return { .length = std::size_t(pos2 - str.data() + 1), .name_length = name_length };
}

namespace {

struct Highlighter {
private:
    Non_Owning_Buffer<Token>& out;
    std::u8string_view remainder;
    Highlight_Options options;

    const std::size_t source_length = remainder.length();
    std::size_t index = 0;

public:
    Highlighter(
        Non_Owning_Buffer<Token>& out,
        std::u8string_view source,
        const Highlight_Options& options
    )
        : out { out }
        , remainder { source }
        , options { options }
    {
    }

private:
    void emit(std::size_t begin, std::size_t length, Highlight_Type type)
    {
        ULIGHT_DEBUG_ASSERT(begin < source_length);
        ULIGHT_DEBUG_ASSERT(begin + length <= source_length);

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

    void advance(std::size_t length)
    {
        index += length;
        remainder.remove_prefix(length);
    }

    void emit_and_advance(std::size_t begin, std::size_t length, Highlight_Type type)
    {
        emit(begin, length, type);
        advance(length);
    }

public:
    bool operator()()
    {
        expect_bom();
        while (!remainder.empty()) {
            if (expect_comment() || //
                expect_doctype() || //
                expect_cdata() || //
                expect_end_tag() || //
                expect_start_tag_permissive() || //
                expect_normal_text()) {
                continue;
            }
            ULIGHT_ASSERT_UNREACHABLE(u8"Unmatched content in HTML.");
        }
        return true;
    }

    bool expect_bom()
    {
        if (remainder.starts_with(byte_order_mark8)) {
            advance(byte_order_mark8.length());
            return true;
        }
        return false;
    }

    bool expect_doctype()
    {
        if (const Match_Result doctype = match_doctype_permissive(remainder)) {
            emit_and_advance(index, doctype.length, Highlight_Type::macro);
            return true;
        }
        return false;
    }

    bool expect_cdata()
    {
        if (const Match_Result cdata = match_cdata(remainder)) {
            emit(index, cdata_prefix.length(), Highlight_Type::macro);
            if (cdata.terminated) {
                emit(
                    index + cdata.length - cdata_suffix.length(), cdata_suffix.length(),
                    Highlight_Type::macro
                );
            }
            advance(cdata.length);
            return true;
        }
        return false;
    }

    bool expect_whitespace()
    {
        if (const std::size_t whitespace_length = match_whitespace(remainder)) {
            advance(whitespace_length);
            return true;
        }
        return false;
    }

    bool expect_comment()
    {
        Match_Result comment = match_comment(remainder);
        if (!comment) {
            return false;
        }
        emit_and_advance(index, comment_prefix.length(), Highlight_Type::comment_delim);
        comment.length -= comment_prefix.length();

        if (comment.terminated) {
            if (comment.length > comment_suffix.length()) {
                emit_and_advance(
                    index, comment.length - comment_suffix.length(), Highlight_Type::comment
                );
            }
            emit_and_advance(index, comment_suffix.length(), Highlight_Type::comment_delim);
        }
        else if (comment.length != 0) {
            emit_and_advance(index, comment.length, Highlight_Type::comment);
        }
        return true;
    }

    bool expect_start_tag_permissive()
    {
        if (!remainder.starts_with(u8'<')) {
            return false;
        }
        emit_and_advance(index, 1, Highlight_Type::sym_punc);

        const std::size_t name_length = match_tag_name(remainder);
        if (name_length == 0) {
            return true;
        }
        const std::u8string_view name = remainder.substr(0, name_length);
        emit_and_advance(index, name_length, Highlight_Type::markup_tag);

        while (!remainder.empty()) {
            expect_whitespace();

            if (remainder.starts_with(u8"/>")) {
                emit_and_advance(index, 2, Highlight_Type::sym_punc);
                break;
            }
            if (remainder.starts_with(u8'>')) {
                emit_and_advance(index, 1, Highlight_Type::sym_punc);
                break;
            }
            if (!expect_attribute()) {
                return true;
            }
        }

        constexpr std::u8string_view textarea_tag = u8"textarea";
        constexpr std::u8string_view title_tag = u8"title";
        constexpr std::u8string_view script_tag = u8"script";
        constexpr std::u8string_view style_tag = u8"style";

        if (equals_ascii_ignore_case(name, textarea_tag)
            || equals_ascii_ignore_case(name, title_tag)) {
            while (const Raw_Text_Result result = match_escapable_raw_text_piece(remainder, name)) {
                advance(result.raw_length);
                if (result.ref_length != 0) {
                    emit_and_advance(index, result.ref_length, Highlight_Type::escape);
                }
            }
            return true;
        }
        if (equals_ascii_ignore_case(name, script_tag)) {
            const std::size_t js_length = match_raw_text(remainder, script_tag);
            // TODO: do nested language highlighting for JavaScript here
            if (js_length != 0) {
                advance(js_length);
            }
            return true;
        }
        if (equals_ascii_ignore_case(name, style_tag)) {
            const std::size_t css_length = match_raw_text(remainder, style_tag);
            // TODO: do nested language highlighting for CSS here
            if (css_length != 0) {
                advance(css_length);
            }
            return true;
        }

        return true;
    }

    bool expect_attribute()
    {
        // https://html.spec.whatwg.org/dev/syntax.html#attributes-2
        const std::size_t name_length = match_attribute_name(remainder);
        if (name_length == 0) {
            return false;
        }
        emit_and_advance(index, name_length, Highlight_Type::markup_attr);
        expect_whitespace();

        // Empty attribute syntax, e.g. <input disabled>
        if (!remainder.starts_with(u8'=')) {
            return true;
        }
        emit_and_advance(index, 1, Highlight_Type::sym_punc);
        expect_whitespace();

        // Always returns true because unquoted (possibly zero-length) is always matched.
        return expect_quoted_attribute_value(u8'\"') //
            || expect_quoted_attribute_value(u8'\'') //
            || expect_unquoted_attribute_value();
    }

    bool expect_unquoted_attribute_value()
    {
        std::size_t piece_length = 0;
        const auto flush = [&] {
            if (piece_length != 0) {
                emit_and_advance(index, piece_length, Highlight_Type::string);
                piece_length = 0;
            }
        };

        for (; piece_length < remainder.length(); ++piece_length) {
            const char8_t c = remainder[piece_length];
            if (is_html_unquoted_attribute_value_terminator(c)) {
                flush();
                return true;
            }
            if (const std::size_t ref_length
                = match_character_reference(remainder.substr(piece_length))) {
                flush();
                emit_and_advance(index, ref_length, Highlight_Type::escape);
            }
        }
        flush();
        return true;
    }

    bool expect_quoted_attribute_value(char8_t quote_char)
    {
        if (!remainder.starts_with(quote_char)) {
            return false;
        }
        std::size_t piece_length = 1;
        const auto flush = [&] {
            if (piece_length != 0) {
                emit_and_advance(index, piece_length, Highlight_Type::string);
                piece_length = 0;
            }
        };

        for (; piece_length < remainder.length(); ++piece_length) {
            const char8_t c = remainder[piece_length];
            if (c == quote_char) {
                ++piece_length;
                flush();
                return true;
            }
            if (const std::size_t ref_length
                = match_character_reference(remainder.substr(piece_length))) {
                flush();
                emit_and_advance(index, ref_length, Highlight_Type::escape);
            }
        }
        flush();
        return true;
    }

    bool expect_end_tag()
    {
        const End_Tag_Result result = match_end_tag_permissive(remainder);
        if (!result) {
            return false;
        }
        ULIGHT_DEBUG_ASSERT(result.name_length != 0);
        emit(index, 2, Highlight_Type::sym_punc); // "</"
        emit(index + 2, result.name_length, Highlight_Type::markup_tag);
        emit(index + result.length - 1, 1, Highlight_Type::sym_punc); // ">"
        advance(result.length);
        return true;
    }

    bool expect_normal_text()
    {
        while (!remainder.empty()) {
            const std::size_t safe_length = remainder.find_first_of(u8"<&");
            if (safe_length == std::u8string_view::npos) {
                advance(remainder.length());
                break;
            }
            if (remainder[safe_length] == u8'<') {
                advance(safe_length);
                break;
            }
            if (!expect_character_reference()) {
                advance(1);
            }
        }
        return true;
    }

    bool expect_character_reference()
    {
        if (const std::size_t ref_length = match_character_reference(remainder)) {
            emit(index, ref_length, Highlight_Type::escape);
            advance(ref_length);
            return true;
        }
        return false;
    }
};

} // namespace

} // namespace html

bool highlight_html( //
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    std::pmr::memory_resource*,
    const Highlight_Options& options
)
{
    return html::Highlighter { out, source, options }();
}

} // namespace ulight
