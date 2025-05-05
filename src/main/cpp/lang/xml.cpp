#include "ulight/impl/lang/xml.hpp"
#include "ulight/impl/ascii_algorithm.hpp"
#include "ulight/impl/lang/html.hpp"
#include "ulight/impl/lang/xml_chars.hpp"

#include "ulight/impl/buffer.hpp"
#include "ulight/impl/highlight.hpp"
#include "ulight/impl/highlighter.hpp"

#include "ulight/impl/unicode_algorithm.hpp"
#include "ulight/ulight.hpp"

#include <cctype>

namespace ulight {

namespace xml {

namespace {

constexpr std::u8string_view comment_prefix = u8"<!--";
constexpr std::u8string_view comment_suffix = u8"-->";
constexpr std::u8string_view illegal_comment_sequence = u8"--";
constexpr std::u8string_view cdata_section_prefix = u8"<![CDATA[";
constexpr std::u8string_view cdata_section_suffix = u8"]]>";
constexpr std::u8string_view tag_suffix = u8">";

[[nodiscard]]
bool contains_xml_string(std::u8string_view str)
{

    for (std::size_t i = 0; i + 2 < str.length(); i++) {
        if ((str[i] == u8'x' || str[i] == u8'X') && //
            (str[i + 1] == u8'm' || str[i + 1] == u8'M') && //
            (str[i + 2] == u8'l' || str[i + 2] == u8'L')) {
            return true;
        }
    }

    return false;
}

} // namespace

[[nodiscard]]
std::size_t match_whitespace(std::u8string_view str)
{
    return ascii::length_if(str, [](char8_t c) { return is_xml_whitespace(c); });
}

[[nodiscard]]
std::size_t match_text(std::u8string_view str)
{
    const std::size_t result
        = ascii::length_if_not(str, [](char32_t c) { return c == u8'<' || c == u8'&'; });

    return result == std::u8string_view::npos ? str.length() : result;
}

[[nodiscard]]
html::Match_Result match_comment(std::u8string_view str)
{
    if (!str.starts_with(comment_prefix)) {
        return {};
    }
    str.remove_prefix(comment_prefix.length());

    std::size_t length = comment_prefix.length();
    while (!str.empty()) {

        if (str.starts_with(comment_suffix)) {
            return { length + comment_suffix.length(), true };
        }

        if (str.starts_with(illegal_comment_sequence)) {
            return { length, false };
        }

        length++;
        str.remove_prefix(1);
    }

    return { length, false };
}

struct XML_Highlighter : Highlighter_Base {

private:
    void highlight_name(const Name_Match_Result& result, Highlight_Type type)
    {
        std::size_t distance = 0;
        for (std::size_t i = 0; i < result.length; i++) {

            if (result.error_indicies.contains(i)) {

                if (distance) {
                    emit_and_advance(distance, type);
                }

                if (!remainder.empty()) {
                    emit_and_advance(1, Highlight_Type::error);
                }

                distance = 0;
                continue;
            }

            distance++;
        }

        if (distance) {
            emit_and_advance(distance, type);
        }
    }

public:
    XML_Highlighter(
        Non_Owning_Buffer<Token>& out,
        std::u8string_view source,
        const Highlight_Options& options
    )
        : Highlighter_Base(out, source, options)
    {
    }

    // TODO: add prolog (declaration)
    bool operator()()
    {
        while (!remainder.empty()) {
            if (expect_comment() || //
                expect_cdata_section() || //
                expect_processing_instruction() || //
                expect_end_tag() || //
                expect_start_tag() || //
                expect_text()) {
                continue;
            }

            ULIGHT_ASSERT_UNREACHABLE(u8"Unmatched XML.");
        }
        return true;
    }

    bool expect_prolog()
    {

        return true;
    }

    bool expect_doctype_decl()
    {

        return false;
    }

    bool expect_processing_instruction()
    {

        if (!remainder.starts_with(u8"<?")) {
            return false;
        }

        emit_and_advance(2, Highlight_Type::sym_punc);
        Name_Match_Result target = match_name_permissive(remainder, [](std::u8string_view str) {
            return !str.empty() && (match_whitespace(str) || str.starts_with(u8"?>"));
        });

        if (!target.length) {
            return true;
        }

        std::u8string_view target_name = remainder.substr(0, target.length);

        if (contains_xml_string(target_name)) {
            return true;
        }

        highlight_name(target, Highlight_Type::macro);

        advance(match_whitespace(remainder));

        while (!remainder.empty() && !remainder.starts_with(u8"?>")) {
            advance(1);
        }

        if (!remainder.starts_with(u8"?>")) {
            return true;
        }

        emit_and_advance(2, Highlight_Type::sym_punc);

        return true;
    }

    bool expect_cdata_section()
    {
        if (const auto cdata_section = html::match_cdata(remainder)) {
            std::size_t pure_cdata_len = cdata_section.length - cdata_section_prefix.length();

            if (cdata_section.terminated) {
                pure_cdata_len -= cdata_section_suffix.length();
            }

            emit_and_advance(cdata_section_prefix.length(), Highlight_Type::macro);

            advance(pure_cdata_len);

            if (cdata_section.terminated) {
                emit_and_advance(cdata_section_suffix.length(), Highlight_Type::macro);
            }

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

        emit_and_advance(ref_length, Highlight_Type::escape);
        return true;
    }

    bool expect_start_tag()
    {

        if (!remainder.starts_with(u8"<")) {
            return false;
        }

        emit_and_advance(1, Highlight_Type::sym_punc);

        Name_Match_Result name = match_name_permissive(remainder, [](std::u8string_view str) {
            return !str.empty()
                && (match_whitespace(str) || str.starts_with(u8"/>") || str.starts_with(tag_suffix)
                );
        });

        if (!name.length) {
            return true;
        }

        highlight_name(name, Highlight_Type::markup_tag);

        while (!remainder.empty()) {

            advance(match_whitespace(remainder));

            if (remainder.starts_with(u8'>') || remainder.starts_with(u8"/>")) {
                break;
            }

            if (!expect_attribute()) {
                break;
            }
        }

        if (remainder.starts_with(u8'>')) {
            emit_and_advance(1, Highlight_Type::sym_punc);
        }
        else if (remainder.starts_with(u8"/>")) {
            emit_and_advance(2, Highlight_Type::sym_punc);
        }

        return true;
    }

    bool expect_attribute()
    {
        Name_Match_Result name = match_name_permissive<>(remainder, [](std::u8string_view str) {
            return !str.empty()
                && (match_whitespace(str) || str.starts_with(u8"/>") || str.starts_with(tag_suffix)
                    || str.starts_with(u8'='));
        });

        highlight_name(name, Highlight_Type::markup_attr);

        advance(match_whitespace(remainder));

        if (!remainder.starts_with(u8'=')) {
            return true;
        }

        emit_and_advance(1, Highlight_Type::sym_punc);

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

        emit_and_advance(1, Highlight_Type::string_delim);

        std::size_t piece_length = 0;

        auto highlight_piece = [&]() {
            if (piece_length && piece_length <= remainder.length()) {
                emit_and_advance(piece_length, Highlight_Type::string);
            }
        };

        while (!remainder.empty() && piece_length < remainder.length()
               && remainder[piece_length] != quote_type) {
            if ((remainder[piece_length] == u8'&' && !html::match_character_reference(remainder))
                || remainder[piece_length] == u8'<') {
                highlight_piece();
                emit_and_advance(1, Highlight_Type::error);
                piece_length = 0;
            }
            else if (remainder[piece_length] == u8'&'
                     && html::match_character_reference(remainder)) {
                highlight_piece();
                expect_reference();
                piece_length = 0;
            }
            else {
                piece_length++;
            }
        }

        highlight_piece();

        if (!remainder.empty() && remainder.starts_with(quote_type)) {
            emit_and_advance(1, Highlight_Type::string_delim);
        }

        return true;
    }

    bool expect_comment()
    {

        html::Match_Result comment = match_comment(remainder);

        if (!comment) {
            return false;
        }

        std::size_t pure_comment_length = comment.length - comment_prefix.length();

        if (comment.terminated) {
            pure_comment_length -= comment_suffix.length();
        }

        // match <--
        emit_and_advance(comment_prefix.length(), Highlight_Type::comment_delim);

        // match actual comment
        if (pure_comment_length > 0) {
            emit_and_advance(pure_comment_length, Highlight_Type::comment);
        }

        // match -->
        if (comment.terminated) {
            emit_and_advance(comment_suffix.length(), Highlight_Type::comment_delim);
        }

        return true;
    }

    bool expect_end_tag()
    {
        if (!remainder.starts_with(u8"</")) {
            return false;
        }

        emit_and_advance(2, Highlight_Type::sym_punc);

        // TODO: add match whitespace back (unrelated to current PR)
        Name_Match_Result name = match_name_permissive(remainder, [](std::u8string_view str) {
            return !str.empty() && (str.starts_with(tag_suffix));
        });

        if (!name.length) {
            return true;
        }

        emit_and_advance(name.length, Highlight_Type::markup_tag);

        advance(match_whitespace(remainder));

        if (!remainder.starts_with(u8'>')) {
            return true;
        }

        emit_and_advance(1, Highlight_Type::sym_punc);

        return true;
    }

    bool expect_text()
    {
        if (const std::size_t text_len = match_text(remainder)) {
            advance(text_len);
            return true;
        }

        if (remainder.starts_with(u8'&') && !expect_reference()) {
            emit_and_advance(1, Highlight_Type::error);
            return true;
        }

        if (remainder.starts_with(u8'<')) {
            emit_and_advance(1, Highlight_Type::error);
            return true;
        }

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
    return xml::XML_Highlighter(out, source, options)();
}

} // namespace ulight
