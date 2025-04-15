#include <string_view>
#include <cstddef>
#include <algorithm>
#include <iterator>
#include <cctype>

#include "ulight/impl/nasm.hpp"
//#include "ulight/impl/highlight.hpp"
//#include "ulight/impl/buffer.hpp"
//#include "ulight/const.hpp"

//#include "ulight/ulight.hpp"




namespace ulight::nasm{


namespace {


/// @brief Characters that valid in labels apart from alphabets and digits
const unsigned char valid_label_characters[] {
    '_',
    '$',
    '#',
    '@',
    '~',
    '.',
    '?'
};


/// @brief Returns true if source line is valid for label consideration, false otherwise
[[nodiscard]]
bool is_valid_label_string(std::u8string_view source){

    const bool isvalid = std::ranges::all_of(source, [](const auto& ch){

        return std::isalnum(static_cast<unsigned char>(ch)) || std::any_of(std::begin(valid_label_characters), std::end(valid_label_characters), [&ch](const auto& sym){
            return ch == sym;
        });
    });
    return isvalid;
}


} // anonymous namespace


/// @brief Returns length of the comment if source line is a comment, 0 otherwise
std::size_t line_comment(std::u8string_view source){
    if(source.starts_with(';')){
        return source.length();
    }
    return 0;
}


/// @brief Returns length if the label if source line is a label, 0 otherwise
std::size_t line_label(std::u8string_view source){

    if(source.empty()){
        return 0;
    }

    if(!source.ends_with(':')){
        return 0;
    }

    if(!(source.starts_with('.') || source.starts_with('_') || source.starts_with('?') || std::isalpha(source[0]))){
        return 0;
    }

    if(!is_valid_label_string(source.substr(0, source.size() - 1))){
        return 0;
    }

    return source.length();
}







/*bool highlight_nasm(
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    std::pmr::memory_resource* memory,
    const Highlight_Options& options
    ){
    return true;
}*/



} // nasm namespace
