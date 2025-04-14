#ifndef ULIGHT_XML_HPP
#define ULIGHT_XML_HPP

#include <cstddef>
#include <string_view>

namespace ulight::xml {

/// @brief returns true if c is whitespace according to the XML standard
[[nodiscard]]
constexpr bool is_xml_whitespace_character(char32_t c);

/// @brief returns true if c is a name start character
/// according to the XML standard
[[nodiscard]]
constexpr bool is_xml_name_start_character(char32_t c);

/// @brief returns true if c is a name character
/// according to the XML standard
[[nodiscard]]
constexpr bool is_xml_name_character(char32_t c);

/// @brief matches whitespace at the beginning of str and returns
/// the length of the matched whitespace. If the start of str
/// is not whitespace 0 is returned
[[nodiscard]]
std::size_t match_whitespace(std::u8string_view str);

/// @brief matches a comment according to the XML standard
/// if the comment is not well formed i.e contains -- besides 
/// the start and end tags 0 is returned.
[[nodiscard]]
std::size_t match_comment(std::u8string_view str);

/// @brief matches a character data section at the start of 
/// str, if the character data section is not
/// well formed we return 0
[[nodiscard]]
std::size_t match_character_data_section(std::u8string_view str);

/// @brief matches a name as defined in the XML standard i.e 
/// a string starting with a name start character followed by 
/// zero or less name characters. If the name is not valid 
/// 0 is returned.
[[nodiscard]]
std::size_t match_name(std::u8string_view str);

/// @brief matches a reference at the beginning of str.
/// A reference can either be a Character Reference or an Entity Reference
[[nodiscard]]
std::size_t match_reference(std::u8string_view str);

/// @brief matches an attribute value according to the XML standard
/// if the attribute value is not well formed we return 0
[[nodiscard]]
std::size_t match_attribute_value(std::u8string_view str);

/// @brief matches a character data section at the beginning of str
/// if there is no character data section at the beginning we return 0
[[nodiscard]]
std::size_t match_cdata_section(std::u8string_view str);

}
#endif



