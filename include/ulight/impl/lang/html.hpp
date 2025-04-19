#ifndef ULIGHT_HTML_HPP
#define ULIGHT_HTML_HPP

#include <cstddef>
#include <string_view>

namespace ulight::html {

struct Match_Result {
    std::size_t length;
    bool terminated;

    [[nodiscard]]
    constexpr explicit operator bool() const
    {
        return length != 0;
    }

    [[nodiscard]]
    friend constexpr bool operator==(Match_Result, Match_Result)
        = default;
};

[[nodiscard]]
std::size_t match_whitespace(std::u8string_view str);

/// @brief Matches a charater reference aka "HTML entity" at the start of `str`.
/// Returns its length (including the leading `&` and trailing `;`),
/// or zero if none could be found.
///
/// Note that this doesn't check whether the character reference is valid.
/// Any `&alphanumeric123;` reference is matched.
[[nodiscard]]
std::size_t match_character_reference(std::u8string_view str);

[[nodiscard]]
std::size_t match_tag_name(std::u8string_view str);

[[nodiscard]]
std::size_t match_attribute_name(std::u8string_view str);

/// @brief Matches a block of raw text starting at `str` and returns its length.
/// That is, text that can appear in raw text elements like `<script>` or `<style>`.
/// @param closing_name The name of the closing tag (usually `"script" or `"style"`).
[[nodiscard]]
std::size_t match_raw_text(std::u8string_view str, std::u8string_view closing_name);

struct Raw_Text_Result {
    /// @brief The length up to the character reference or raw text end, in code units.
    std::size_t raw_length;
    /// @brief The length of the character reference, if any.
    std::size_t ref_length;

    [[nodiscard]]
    constexpr explicit operator bool() const
    {
        return raw_length != 0 || ref_length != 0;
    }

    [[nodiscard]]
    friend constexpr bool operator==(Raw_Text_Result, Raw_Text_Result)
        = default;
};

/// @brief Matches a block of escapable raw text.
/// This is almost identical to `match_raw_text`,
/// except that the text can contain character references.
[[nodiscard]]
Raw_Text_Result
match_escapable_raw_text_piece(std::u8string_view str, std::u8string_view closing_name);

/// @brief Matches a comment exactly according to the HTML standard.
/// Note that HTML comments are in some ways more restrictive,
/// and in some ways less restrictive than XML comments.
///
/// For example, HTML comments cannot begin with `<!-->`, but they can in XML.
/// XML comments cannot contain `--` anywhere, but HTML comments can.
[[nodiscard]]
Match_Result match_comment(std::u8string_view str);

/// @brief Matches a DOCTYPE at the start of `str`.
///
/// This function is more permissive than the actual restrictions for DOCTYPEs in the HTML
/// standard.
/// Any contents between `<!DOCTYPE` and `>` is accepted.
[[nodiscard]]
Match_Result match_doctype_permissive(std::u8string_view str);

/// @brief Matches CDATA at the start of `str`.
[[nodiscard]]
Match_Result match_cdata(std::u8string_view str);

struct End_Tag_Result {
    /// @brief The total length of the tag, in code units,
    /// including the opening `</` and closing `>`.
    std::size_t length;
    /// @brief The length of the tag name, in code units.
    std::size_t name_length;

    [[nodiscard]]
    constexpr explicit operator bool() const
    {
        return length != 0;
    }

    [[nodiscard]]
    friend constexpr bool operator==(End_Tag_Result, End_Tag_Result)
        = default;
};

/// @brief Matches an HTML end tag, like `</b>` at the start of `str`.
///
/// This does not validate whether the tag name is valid.
[[nodiscard]]
End_Tag_Result match_end_tag_permissive(std::u8string_view str);

} // namespace ulight::html

#endif
