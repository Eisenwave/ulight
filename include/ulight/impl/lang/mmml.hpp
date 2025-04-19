#ifndef ULIGHT_MMML_HPP
#define ULIGHT_MMML_HPP

#include <cstddef>
#include <string_view>

namespace ulight::mmml {

[[nodiscard]]
std::size_t match_directive_name(std::u8string_view str);

[[nodiscard]]
std::size_t match_argument_name(std::u8string_view str);

[[nodiscard]]
std::size_t match_whitespace(std::u8string_view str);

[[nodiscard]]
bool starts_with_escape_or_directive(std::u8string_view str);

struct Named_Argument_Result {
    std::size_t length;
    std::size_t leading_whitespace;
    std::size_t name_length;
    std::size_t trailing_whitespace;

    [[nodiscard]]
    constexpr explicit operator bool() const
    {
        return length != 0;
    }

    friend constexpr bool operator==(const Named_Argument_Result&, const Named_Argument_Result&)
        = default;
};

[[nodiscard]]
Named_Argument_Result match_named_argument_prefix(std::u8string_view str);

} // namespace ulight::mmml

#endif
