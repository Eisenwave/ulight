#ifndef ULIGHT_CSS_HPP
#define ULIGHT_CSS_HPP

#include <cstddef>
#include <string_view>

#include "ulight/impl/platform.h"

namespace ulight::css {

[[nodiscard]]
bool starts_with_number(std::u8string_view str);

[[nodiscard]]
bool starts_with_valid_escape(std::u8string_view str);

[[nodiscard]]
bool starts_with_ident_sequence(std::u8string_view str);

[[nodiscard]]
std::size_t match_number(std::u8string_view str);

[[nodiscard]]
std::size_t match_escaped_code_point(std::u8string_view str);

[[nodiscard]]
std::size_t match_ident_sequence(std::u8string_view str);

enum struct Ident_Type : Underlying {
    /// @brief CSS `ident-token`.
    ident,
    /// @brief CSS `function-token`.
    function,
    /// @brief CSS `url-token` or `bad-url-token`.
    url,
};

[[nodiscard]]
constexpr std::u8string_view enumerator_of(Ident_Type type)
{
    switch (type) {
        using enum Ident_Type;
    case ident: return u8"ident";
    case function: return u8"function";
    case url: return u8"url";
    }
    return {};
}

struct Ident_Result {
    std::size_t length;
    /// @brief The type of the token.
    Ident_Type type;

    [[nodiscard]]
    explicit operator bool() const
    {
        return length != 0;
    }

    [[nodiscard]]
    friend constexpr bool operator==(Ident_Result, Ident_Result)
        = default;
};

[[nodiscard]]
Ident_Result match_ident_like_token(std::u8string_view str);

} // namespace ulight::css

#endif
