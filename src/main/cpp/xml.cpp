#include "ulight/impl/xml.hpp"
#include "ulight/impl/buffer.hpp"
#include "ulight/impl/chars.hpp"
#include "ulight/impl/highlight.hpp"
#include "ulight/impl/html.hpp"
#include "ulight/ulight.hpp"
#include <cctype>
#include <cmath>
#include <functional>
#include <string>

namespace ulight {

namespace xml {

constexpr std::u8string_view comment_prefix = u8"<!--";
constexpr std::u8string_view comment_suffix = u8"-->";
constexpr std::u8string_view illegal_comment_sequence = u8"--";
constexpr std::u8string_view cdata_section_prefix = u8"<![CDATA[";
constexpr std::u8string_view cdata_section_suffix = u8"]]>";

[[nodiscard]]
constexpr bool is_xml_whitespace_character(char32_t c)
{

    return c == U'\u0009' || c == U'\u0020' || c == U'\u000d' || c == U'\u000a';
}

[[nodiscard]]
constexpr bool is_xml_name_start_character(char32_t c)
{

    return is_ascii_alpha(c) //
        || c == U':' //
        || c == U'_' //
        || (c >= U'\u00c0' && c <= U'\u00d6') //
        || (c >= U'\u00d8' && c <= U'\u00f6') //
        || (c >= U'\u00f8' && c <= U'\u02ff') //
        || (c >= U'\u0370' && c <= U'\u037d') //
        || (c >= U'\u037f' && c <= U'\u1fff') //
        || (c >= U'\u200c' && c <= U'\u200d') //
        || (c >= U'\u2070' && c <= U'\u218f') //
        || (c >= U'\u2c00' && c <= U'\u2fef') //
        || (c >= U'\u3001' && c <= U'\ud7ff') //
        || (c >= U'\uf900' && c <= U'\ufdcf') //
        || (c >= U'\ufdf0' && c <= U'\ufffd');
}

// could maybe use html tag characters
[[nodiscard]]
constexpr bool is_xml_name_character(char32_t c)
{

    return is_ascii_digit(c) //
        || is_xml_name_start_character(c) //
        || c == U':' //
        || c == U'.' //
        || c == U'\u00b7' //
        || (c >= U'\u0300' && c <= U'\u036f') //
        || (c >= U'\u203f' && c <= U'\u2040');
}

[[nodiscard]]
std::size_t match_whitespace(std::u8string_view str)
{
    std::size_t length = 0;
    while (!str.empty() && is_xml_whitespace_character(str[length])) {
        length++;
    }

    return length;
}

[[nodiscard]]
std::size_t match_name(std::u8string_view str)
{

    std::size_t length = 0;
    if (str.empty() || !is_xml_name_start_character(str[0])) {
        return 0;
    }

    length++;
    while (length < str.length() && is_xml_name_character(str[length])) {
        length++;
    }

    return length;
}

[[nodiscard]]
std::size_t match_comment(std::u8string_view str)
{

    if (!str.starts_with(comment_prefix)) {
        return 0;
    }

    str.remove_prefix(comment_prefix.length());
    std::size_t has_end = str.find(comment_suffix);

    if (has_end == std::string::npos) {
        return 0;
    }

    std::size_t length = comment_prefix.length();
    while (!str.empty()) {

        if (str.starts_with(comment_suffix)) {
            return length + comment_suffix.length();
        }

        if (str.starts_with(illegal_comment_sequence)) {
            return 0;
        }

        length++;
        str.remove_prefix(1);
    }

    return length;
}

[[nodiscard]]
std::size_t match_cdata_section(std::u8string_view str)
{

    if (!str.starts_with(cdata_section_prefix)) {
        return 0;
    }

    std::size_t has_end = str.find_first_of(cdata_section_suffix);

    if (has_end == std::string::npos) {
        return 0;
    }

    return has_end + cdata_section_suffix.length();
}

struct Decl_Info_Match_Result {

    std::size_t leading_whitespace_len;
    std::size_t before_eq_whitespace_len;
    std::size_t after_eq_whitespace_len;
    std::size_t content_len;
    bool terminated = false;
};

[[nodiscard]]
std::size_t match_version_num(std::u8string_view str)
{
    std::size_t version_num_len = 0;
    if (!str.starts_with(u8"1.")) {
        return 0;
    }

    version_num_len += 2;
    str.remove_prefix(2);

    while (!str.empty() && is_ascii_digit(str.front())) {
        version_num_len++;
        str.remove_prefix(1);
    }

    return version_num_len;
}

[[nodiscard]]
std::size_t match_encoding_name(std::u8string_view str)
{
    std::size_t encoding_name_len = 0;

    if (str.empty() || !is_ascii_alpha(str.front())) {
        return 0;
    }

    encoding_name_len++;
    str.remove_prefix(1);

    while (!str.empty()
           && (is_ascii_alpha(str.front()) || is_ascii_digit(str.front()) || str.front() == u8'.'
               || str.front() == u8'_' || str.front() == u8'-')) {
        encoding_name_len++;
        str.remove_prefix(1);
    }

    return encoding_name_len;
}

[[nodiscard]]
std::size_t match_standalone_option(std::u8string_view str)
{
    if (str.starts_with(u8"yes")) {
        return 3;
        ;
    }
    if (str.starts_with(u8"no")) {
        return 2;
    }

    return 0;
}

using Matcher_Type = std::function<std::size_t(std::u8string_view)>;

[[nodiscard]]
Decl_Info_Match_Result match_xml_decl_info(
    std::u8string_view str,
    std::u8string_view decl_type,
    const Matcher_Type& val_matcher
)
{

    Decl_Info_Match_Result res { 0, 0, 0, 0 };

    res.leading_whitespace_len = match_whitespace(str);
    str.remove_prefix(match_whitespace(str));

    if (!str.starts_with(decl_type) || !res.leading_whitespace_len) {
        return {};
    }

    str.remove_prefix(decl_type.length());

    res.before_eq_whitespace_len += match_whitespace(str);
    str.remove_prefix(match_whitespace(str));

    if (!str.starts_with(u8'=')) {
        return {};
    }
    str.remove_prefix(1);

    res.after_eq_whitespace_len += match_whitespace(str);
    str.remove_prefix(match_whitespace(str));

    char8_t quote_type;
    if (str.starts_with(u8'\'')) {
        quote_type = u8'\'';
    }
    else if (str.starts_with(u8'\"')) {
        quote_type = u8'\"';
    }
    else {
        return {};
    }
    str.remove_prefix(1);

    std::size_t content_len = val_matcher(str);
    str.remove_prefix(content_len);

    if (!content_len || !str.starts_with(quote_type)) {
        return {};
    }

    res.content_len = content_len + 2;
    res.terminated = true;
    return res;
}

// yoinked from html module, liked the idea
struct XML_Highlighter {

private:
    std::u8string_view remainder;
    Non_Owning_Buffer<Token>& out;
    Highlight_Options options;

    const std::size_t source_length = remainder.length();
    std::size_t index = 0;

public:
    XML_Highlighter(
        Non_Owning_Buffer<Token>& out,
        std::u8string_view source,
        const Highlight_Options& options
    )
        : remainder { source }
        , out { out }
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
    // TODO: add prolog
    bool highlight()
    {

        if (!expect_prolog()) {
            ULIGHT_ASSERT_UNREACHABLE(u8"Unmatched XML");
        }

        while (!remainder.empty()) {
            if (expect_comment() || //
                expect_cdata() || //
                expect_processing_instruction() || //
                expect_end_tag() || //
                expect_start_tag() || //
                expect_reference()) {
                continue;
            }

            ULIGHT_ASSERT_UNREACHABLE(u8"Unmatched XML.");
        }
        return true;
    }

    bool expect_prolog()
    {

        return expect_xml_decl();
    }

    bool expect_xml_decl()
    {

        if (!remainder.starts_with(u8"<?xml")) {
            return false;
        }

        emit_and_advance(index, 5, Highlight_Type::macro);

        Decl_Info_Match_Result version_info
            = match_xml_decl_info(remainder, u8"version", match_version_num);

        if (!version_info.terminated) {
            return false;
        }

        auto highlight = [this](std::size_t decl_type_len, Decl_Info_Match_Result res) {
            this->advance(res.leading_whitespace_len);
            this->emit_and_advance(index, decl_type_len, Highlight_Type::markup_attr);
            this->advance(res.before_eq_whitespace_len);
            this->emit_and_advance(index, 1, Highlight_Type::sym_punc);
            this->advance(res.after_eq_whitespace_len);
            this->emit_and_advance(index, res.content_len, Highlight_Type::string);
        };

        highlight(7, version_info);

        Decl_Info_Match_Result encoding_decl
            = match_xml_decl_info(remainder, u8"encoding", match_encoding_name);
        if (encoding_decl.terminated) {
            highlight(8, encoding_decl);
        }

        Decl_Info_Match_Result standalone_decl
            = match_xml_decl_info(remainder, u8"standalone", match_standalone_option);
        if (standalone_decl.terminated) {
            highlight(10, standalone_decl);
        }

        advance(match_whitespace(remainder));

        if (remainder.starts_with(u8"?>")) {
            emit_and_advance(index, 2, Highlight_Type::macro);
            return true;
        }

        return false;
    }

    bool expect_processing_instruction()
    {

        if (!remainder.starts_with(u8"<?")) {
            return false;
        }

        emit_and_advance(index, 2, Highlight_Type::sym_punc);
        std::size_t target_length = match_name(remainder);

        if (!target_length) {
            return false;
        }

        std::u8string_view target = remainder.substr(index, target_length);
        emit_and_advance(index, target_length, Highlight_Type::macro);

        // TODO make sure it does not contain (x|X)(m|M)(l|L)
        advance(match_whitespace(remainder));

        std::size_t inst_len = 0;
        while (inst_len < remainder.length() && !remainder.substr(inst_len).starts_with(u8"?>")) {
            inst_len++;
        }

        if (inst_len == remainder.length()) {
            return false;
        }

        advance(inst_len);

        emit_and_advance(index, 2, Highlight_Type::sym_punc);

        return true;
    }

    bool expect_cdata()
    {
        // could reuse html version of match cdata section
        if (std::size_t cdata_section = match_cdata_section(remainder)) {
            std::size_t pure_cdata_len
                = cdata_section - cdata_section_prefix.length() - cdata_section_suffix.length();

            emit_and_advance(index, cdata_section_prefix.length(), Highlight_Type::macro);

            advance(pure_cdata_len);

            emit_and_advance(index, cdata_section_suffix.length(), Highlight_Type::macro);

            return true;
        }

        // avoid copying
        std::u8string_view temp = remainder;
        std::size_t length = 0;
        while (!temp.empty() && !temp.starts_with(cdata_section_suffix) && !temp.starts_with(u8'<')
               && !temp.starts_with(u8'&')) {
            temp.remove_prefix(1);
            length++;
        }

        if (length) {
            advance(length);
            return true;
        }

        return false;
    }

    bool expect_reference()
    {

        std::size_t ref_length = html::match_character_reference(remainder);

        if (!ref_length) {
            return false;
        }

        emit_and_advance(index, ref_length, Highlight_Type::escape);
        return true;
    }

    // either start tag or empty element tag
    // maybe explicit for empty element tag ?
    bool expect_start_tag()
    {

        if (!remainder.starts_with(u8"<")) {
            return false;
        }

        emit_and_advance(index, 1, Highlight_Type::sym_punc);

        std::size_t name_length = match_name(remainder);

        if (!name_length) {
            return false;
        }

        emit_and_advance(index, name_length, Highlight_Type::markup_tag);

        while (!remainder.empty()) {
            std::size_t whitespace_length = match_whitespace(remainder);
            advance(whitespace_length);

            if (remainder.starts_with(u8'>') || remainder.starts_with(u8"/>")) {
                break;
            }

            if (!expect_attribute()) {
                return false;
            }
        }

        if (remainder.starts_with(u8'>')) {
            emit_and_advance(index, 1, Highlight_Type::sym_punc);
            return true;
        }

        if (remainder.starts_with(u8"/>")) {
            emit_and_advance(index, 2, Highlight_Type::sym_punc);
            return true;
        }

        return false;
    }

    bool expect_attribute()
    {

        std::size_t name_length = match_name(remainder);

        if (!name_length) {
            return false;
        }

        emit_and_advance(index, name_length, Highlight_Type::markup_attr);

        advance(match_whitespace(remainder));

        if (!remainder.starts_with(u8'=')) {
            return false;
        }

        advance(1);

        advance(match_whitespace(remainder));

        return expect_attribute_value();
    }

    bool expect_attribute_value()
    {

        char8_t quote_type;
        if (remainder.starts_with(u8'\'')) {
            quote_type = u8'\'';
        }
        else if (remainder.starts_with(u8'\"')) {
            quote_type = u8'\"';
        }
        else {
            return false;
        }

        advance(1);
        std::size_t attval_len = 0;
        while (attval_len < remainder.length() && remainder[attval_len] != quote_type) {
            if (remainder[attval_len] == u8'&') {
                emit_and_advance(index, attval_len, Highlight_Type::string);
                if (!expect_reference()) {
                    return false;
                }
                attval_len = 0;
            }
            else {
                attval_len++;
            }
        }

        if (remainder.empty()) {
            return false;
        }

        // is this safe ?
        emit_and_advance(index, attval_len + 1, Highlight_Type::string);

        return true;
    }

    bool expect_comment()
    {
        std::size_t comment_length = match_comment(remainder);

        if (!comment_length) {
            return false;
        }

        std::size_t pure_comment_length
            = comment_length - comment_prefix.length() - comment_suffix.length();

        // <!--
        emit_and_advance(index, comment_prefix.length(), Highlight_Type::comment_delimiter);

        // actual comment
        emit_and_advance(index, pure_comment_length, Highlight_Type::comment);

        // -->
        emit_and_advance(index, comment_suffix.length(), Highlight_Type::comment_delimiter);

        return true;
    }

    bool expect_end_tag()
    {

        if (!remainder.starts_with(u8"</")) {
            return false;
        }

        emit_and_advance(index, 2, Highlight_Type::sym_punc);

        std::size_t name_length = match_name(remainder);

        if (!name_length) {
            return false;
        }

        emit_and_advance(index, name_length, Highlight_Type::markup_tag);

        if (!remainder.starts_with(u8'>')) {
            return false;
        }

        emit_and_advance(index, 1, Highlight_Type::sym_punc);

        return true;
    }
};

} // namespace xml

bool highlight_xml(
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    std::pmr::memory_resource*,
    const Highlight_Options& options
)
{
    return xml::XML_Highlighter(out, source, options).highlight();
}

} // namespace ulight
