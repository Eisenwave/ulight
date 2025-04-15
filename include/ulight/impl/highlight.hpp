#ifndef ULIGHT_HIGHLIGHT_TOKEN_HPP
#define ULIGHT_HIGHLIGHT_TOKEN_HPP

#include <memory_resource>
#include <string_view>

#include "ulight/ulight.hpp"

#include "ulight/impl/buffer.hpp"

namespace ulight {

struct Highlight_Options {
    /// @brief If `true`,
    /// adjacent spans with the same `Highlight_Type` get merged into one.
    bool coalescing = false;
    /// @brief If `true`,
    /// does not highlight keywords and other features from technical specifications,
    /// compiler extensions, from similar languages, and other "non-standard" sources.
    ///
    /// For example, if `false`, C++ highlighting also includes all C keywords.
    bool strict = false;
};

bool highlight_mmml(
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    std::pmr::memory_resource* memory,
    const Highlight_Options& options = {}
);
bool highlight_cpp(
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    std::pmr::memory_resource* memory,
    const Highlight_Options& options = {}
);
bool highlight_lua(
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    std::pmr::memory_resource* memory,
    const Highlight_Options& options = {}
);
bool highlight_html(
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    std::pmr::memory_resource* memory,
    const Highlight_Options& options = {}
);
bool highlight_css(
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    std::pmr::memory_resource* memory,
    const Highlight_Options& options = {}
);
bool highlight_c(
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    std::pmr::memory_resource* memory,
    const Highlight_Options& options = {}
);
bool highlight_javascript(
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    std::pmr::memory_resource* memory,
    const Highlight_Options& options = {}
);

bool highlight_nasm(
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    std::pmr::memory_resource* memory,
    const Highlight_Options& options = {}
);


inline Status highlight(
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    Lang language,
    std::pmr::memory_resource* memory,
    const Highlight_Options& options = {}
)
{
    constexpr auto to_result = [](bool success) -> Status {
        if (success) {
            return Status::ok;
        }
        return Status::bad_code;
    };

    switch (language) {
    case Lang::cpp: //
        return to_result(highlight_cpp(out, source, memory, options));
    case Lang::mmml: //
        return to_result(highlight_mmml(out, source, memory, options));
    case Lang::lua: //
        return to_result(highlight_lua(out, source, memory, options));
    case Lang::html: //
        return to_result(highlight_html(out, source, memory, options));
    case Lang::css: //
        return to_result(highlight_css(out, source, memory, options));
    case Lang::c: //
        return to_result(highlight_c(out, source, memory, options));
    case Lang::javascript: //
        return to_result(highlight_javascript(out, source, memory, options));
    //case Lang::nasm:
    //    return to_result(highlight_nasm(out, source, memory, options));
    default: //
        return Status::bad_lang;
    }
}

} // namespace ulight

#endif
