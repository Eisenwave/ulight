#ifndef ULIGHT_XML_HPP
#define ULIGHT_XML_HPP

#include <cstddef>
#include <string_view>

#include "html.hpp"

namespace ulight::xml {

/// @brief matches whitespace at the beginning of str and returns
/// the length of the matched whitespace. If the start of str
/// is not whitespace 0 is returned
[[nodiscard]]
std::size_t match_whitespace(std::u8string_view str);

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
