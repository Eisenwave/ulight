#include "ulight/impl/xml.hpp"
#include "ulight/impl/buffer.hpp"
#include "ulight/impl/chars.hpp"
#include "ulight/impl/highlight.hpp"
#include "ulight/ulight.hpp"

#include <string>

namespace ulight {

namespace xml {

constexpr std::u8string_view comment_prefix = u8"<!--";
constexpr std::u8string_view comment_suffix = u8"-->";
constexpr std::u8string_view illegal_comment_sequence = u8"--";
constexpr std::u8string_view char_ref_start_dec = u8"&#";
constexpr std::u8string_view char_ref_start_hex = u8"&#x";

[[nodiscard]]
constexpr bool is_xml_whitespace_character(char32_t c) {

  return c == U'\u0009'
    || c == U'\u0020'
    || c == U'\u000d'
    || c == U'\u000a';
}

[[nodiscard]]
constexpr bool is_xml_name_start_character(char32_t c) {

  return is_ascii_alpha(c)
    || c == U':'
    || c == U'_'
    || (c >= U'\u00c0' && c <= U'\u00d6')
    || (c >= U'\u00d8' && c <= U'\u00f6')
    || (c >= U'\u00f8' && c <= U'\u02ff')
    || (c >= U'\u0370' && c <= U'\u037d')
    || (c >= U'\u037f' && c <= U'\u1fff')
    || (c >= U'\u200c' && c <= U'\u200d')
    || (c >= U'\u2070' && c <= U'\u218f')
    || (c >= U'\u2c00' && c <= U'\u2fef')
    || (c >= U'\u3001' && c <= U'\ud7ff')
    || (c >= U'\uf900' && c <= U'\ufdcf')
    || (c >= U'\ufdf0' && c <= U'\ufffd');
}

// could maybe use html tag characters
[[nodiscard]]
constexpr bool is_xml_name_character(char32_t c) {

  return is_ascii_digit(c) 
    || is_xml_name_start_character(c)
    || c == U':'
    || c == U'.'
    || c == U'\u00b7'
    || (c >= U'\u0300' && c <= U'\u036f')
    || (c >= U'\u203f' && c <= U'\u2040');
}

[[nodiscard]]
std::size_t match_whitespace(std::u8string_view str) {
  std::size_t length = 0;
  while (!str.empty() && is_xml_whitespace_character(str[length])) {
    length++; 
  }

  return length;
}

[[nodiscard]]
std::size_t match_name(std::u8string_view str) {

  std::size_t length = 0;
  if (str.empty() || !is_xml_name_start_character(str[0])) {
    return 0;
  }

  length++;
  while (is_xml_name_character(str[length])) {
    length++;
  }

  return length;
}

// check which length to return 
// and where to start highlighting
[[nodiscard]]
std::size_t match_reference(std::u8string_view str) {

  std::size_t length = 0;
  if (str.starts_with(char_ref_start_hex)) {

    // character reference in hex
    str.remove_prefix(char_ref_start_hex.length()); 

    while (is_ascii_hex_digit(str[0])) {
      str.remove_prefix(1);
      length++;
    }
    
    length =  length ? length + char_ref_start_hex.length() : 0;

  } else if (str.starts_with(char_ref_start_dec)) {

    // character reference in decimal
    str.remove_prefix(char_ref_start_dec.length());

    while (is_ascii_digit(str[0])) {
      str.remove_prefix(1);
      length++;
    }

    length = length ? length + char_ref_start_dec.length() : 0;
  } else if (str.starts_with(u8'&')) {

    // enitity reference
    str.remove_prefix(1);
    length = match_name(str);

    if (!length) {
      return 0;
    }

    str.remove_prefix(length);

    length = length + 1;
  }

  if (!str.starts_with(u8";")) {
      return 0;
  }

  return length + 1;
}

[[nodiscard]]
std::size_t match_attribute_value_quoted(std::u8string_view str, char8_t quote_type) {

  // remove starting quote
  str.remove_prefix(1);

  std::size_t length = 0;
  while (!str.empty()) {

    if (str.starts_with(quote_type)) {
      return length + 2;
    } 

    if (str.starts_with(u8'>')) {
      return 0;
    }

    if (str.starts_with(u8'&')) {
      std::size_t ref_length = match_reference(str);
      
      if (!ref_length) {
        return 0;
      }

      str.remove_prefix(ref_length);
      length += ref_length;
    } else {
      str.remove_prefix(1);
      length++;
    }
  }

  return length;
}

[[nodiscard]]
std::size_t match_attribute_value(std::u8string_view str) {  

  if (str.starts_with(u8"\"")) {
    return match_attribute_value_quoted(str, u8'\"');
  } 

  if (str.starts_with(u8"\'")) {
    return match_attribute_value_quoted(str, u8'\'');
  }

  return 0;
}

[[nodiscard]]
std::size_t match_comment(std::u8string_view str) {

  if(!str.starts_with(comment_prefix)) {
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

// actually ripped from html "module", liked the idea
// is there some genralization ?
struct XML_Highlighter {

private:

  std::u8string_view remainder;
  Non_Owning_Buffer<Token> out;
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
    // TODO: add element content 
    bool highlight() {

      expect_prolog();
      while (!remainder.empty()) {
        if (expect_start_tag() ||
            expect_comment() ||
            expect_end_tag()) {
          continue;
        } else {

          return false;
        }
      }

      return true;
    }

    bool expect_prolog() {


    }

    bool expect_start_tag() {

      if (!remainder.starts_with(u8"<")) {
        return false;
      }

      emit_and_advance(index, 1, Highlight_Type::sym_punc);

      std::size_t name_length = match_name(remainder);

      if (!name_length) {
        return false;
      }

      emit_and_advance(index, name_length, Highlight_Type::id);
      
      while (!remainder.empty()) {
        std::size_t whitespace_length = match_whitespace(remainder);        
        advance(whitespace_length);

        if (remainder.starts_with(u8">") || 
            remainder.starts_with(u8"/>")) {
          break;
        }

        if (!expect_attribute()) {
          return false;
        }
      }

      if (remainder.starts_with(u8">")) {
        emit_and_advance(index, 1, Highlight_Type::sym_punc);
        return true;
      }

      if (remainder.starts_with(u8"/>")) {
        emit_and_advance(index, 2, Highlight_Type::sym_punc);
        return true;
      }

      return false;;
    }

    bool expect_attribute() {
 
      std::size_t name_length = match_name(remainder);

      if (!name_length) {
        return false;
      }

      emit_and_advance(index, name_length, Highlight_Type::id);

      advance(match_whitespace(remainder));

      if (!remainder.starts_with(u8"=")) {
        return false;
      }

      advance(match_whitespace(remainder));

      std::size_t attval_length = match_attribute_value(remainder);
     
      if (!attval_length) {
        return false;
      }

      emit_and_advance(index, attval_length, Highlight_Type::string);

      return true;
    }

    bool expect_comment() {
      std::size_t comment_length = match_comment(remainder);
      
      if (!comment_length) {
        return false;
      }

      emit_and_advance(index, comment_length, Highlight_Type::comment);
      
      return true;
    }


    bool expect_end_tag() {

      if (!remainder.starts_with(u8"</")) {
        return false;
      }

      emit_and_advance(index, 2, Highlight_Type::sym_punc);

      std::size_t name_length = match_name(remainder);

      if (!name_length) {
        return false;
      }

      emit_and_advance(index, name_length, Highlight_Type::id);

      if (!remainder.starts_with(u8"/>")) {
        return false;
      }

      emit_and_advance(index, 2, Highlight_Type::sym_punc);

      return true;
    }

};


} // namespace xml

bool highlight_xml(
    Non_Owning_Buffer<Token> &out,
    std::u8string_view source, 
    std::pmr::memory_resource*,
    const Highlight_Options& options
) 
{
    return xml::XML_Highlighter(out, source, options).highlight();
}


} // namespace ulight



