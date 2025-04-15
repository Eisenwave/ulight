#ifndef NASM_HPP
#define NASM_HPP

#include <cstddef>
#include <string_view>


namespace ulight::nasm{


/// @brief

[[nodiscard]]
std::size_t line_comment(std::u8string_view source);

[[nodiscard]]
std::size_t line_label(std::u8string_view source);
} // nasm namespace


#endif // NASM_HPP
