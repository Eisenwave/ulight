#ifndef ULIGHT_XML_HPP
#define ULIGHT_XML_HPP

#include <cstddef>
#include <functional>
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
std::size_t match_cdata_section(std::u8string_view str);

/// @brief matches a name as defined in the XML standard i.e 
/// a string starting with a name start character followed by 
/// zero or less name characters. If the name is not valid 
/// 0 is returned.
[[nodiscard]]
std::size_t match_name(std::u8string_view str);

struct Decl_Info_Match_Result;
using Matcher_Type = std::function<std::size_t(std::u8string_view)>;

/// @brief matches the xml version number at the beginning of str
[[nodiscard]]
std::size_t match_version_num(std::u8string_view str);

/// @brief matches the name of an encoding scheme 
/// at the beginning of str as stated in the XML standard.
[[nodiscard]]
std::size_t match_encoding_name(std::u8string_view str);

/// @brief matches the yes/no at the beginning of str 
[[nodiscard]]
std::size_t match_standalone_option(std::u8string_view str);

/// @brief matches declaration information for decl_type at the beginning 
/// of str and uses val_matcher to match match the value for that type.
[[nodiscard]]
Decl_Info_Match_Result match_xml_decl_info(std::u8string_view str, std::u8string_view decl_type, const Matcher_Type& val_matcher);

} // namespace ulight::xml
#endif



