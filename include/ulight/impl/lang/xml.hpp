#ifndef ULIGHT_XML_HPP
#define ULIGHT_XML_HPP

#include <cstddef>
#include <set>
#include <string_view>
#include <type_traits>

#include "ulight/impl/lang/xml_chars.hpp"
#include "ulight/impl/unicode_algorithm.hpp"

#include "html.hpp"

namespace ulight::xml {

/// @brief matches whitespace at the beginning of str and returns
/// the length of the matched whitespace. If the start of str
/// is not whitespace 0 is returned
[[nodiscard]]
std::size_t match_whitespace(std::u8string_view str);

struct Name_Match_Result {
    std::size_t length = 0;
    std::set<std::size_t> error_indicies;
};

/// @brief matches a name at the beginning of str until the
/// beginning of str satisfies the predicate is_stop_sequence.
/// Characters that are not allowed in a name e.g & and > are
/// marked for later highlighting.
template <typename Stop>
[[nodiscard]]
Name_Match_Result match_name_permissive(const std::u8string_view str, Stop is_stop)
{
    Name_Match_Result result {};

    while (result.length < str.length() && !is_stop(str.substr(result.length))) {
        const auto [code_point, length]
            = utf8::decode_and_length_or_throw(str.substr(result.length));
        if ((result.length == 0 && !is_xml_name_start(code_point)) || !is_xml_name(code_point)) {
            result.error_indicies.insert(result.length);
        }
        result.length += std::size_t(length);
    }

    return result;
}

/// @brief matches a comment at the start of str
/// according to the XML standard
/// in a optimistic way, i.e if str starts with <!--
/// is considered a comment up until --> or a sequence
/// that is not allowed in comments
[[nodiscard]]
html::Match_Result match_comment(std::u8string_view str);

/// @brief matches character data at the beginning of str.
/// The character data that is being matched needs to be character
/// data according to the XML standard i.e must not contain
/// '&' or '<'
[[nodiscard]]
std::size_t match_text(std::u8string_view str);

} // namespace ulight::xml
#endif
