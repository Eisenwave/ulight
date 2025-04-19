#include <string_view>

#include "ulight/impl/buffer.hpp"
#include "ulight/impl/highlight.hpp"
#include "ulight/impl/highlighter.hpp"
#include "ulight/impl/parse_utils.hpp"

#include "ulight/impl/lang/diff.hpp"

namespace ulight {
namespace diff {

Highlight_Type choose_line_highlight(std::u8string_view line)
{
    // https://www.gnu.org/software/diffutils/manual/html_node/Detailed-Unified.html
    ULIGHT_ASSERT(!line.empty());
    switch (line[0]) {
    case u8'-':
        return line.starts_with(u8"--- ") ? Highlight_Type::diff_heading
                                          : Highlight_Type::diff_deletion;
    case u8'+':
        return line.starts_with(u8"+++ ") ? Highlight_Type::diff_heading
                                          : Highlight_Type::diff_insertion;
    case u8'*':
        return line.starts_with(u8"*** ")
                || line.find_first_not_of(u8'*') == std::u8string_view::npos
            ? Highlight_Type::diff_heading
            : Highlight_Type::diff_common;
    case u8'!': return Highlight_Type::diff_modification;
    case u8'@':
        return line.starts_with(u8"@@ ") ? Highlight_Type::diff_heading_hunk
                                         : Highlight_Type::diff_common;
    default: return Highlight_Type::diff_common;
    }
}

namespace {

struct Highlighter : Highlighter_Base {

    Highlighter(
        Non_Owning_Buffer<Token>& out,
        std::u8string_view source,
        const Highlight_Options& options
    )
        : Highlighter_Base { out, source, options }
    {
    }

    bool operator()()
    {
        while (!remainder.empty()) {
            const Line_Result line = match_line(remainder);
            // If there are remaining characters in the file,
            // how could there not be a remaining line?!
            ULIGHT_ASSERT(line.content_length != 0 || line.terminator_length != 0);
            highlight_line(remainder.substr(0, line.content_length));
            advance(line.terminator_length);
        }
        return true;
    }

    void highlight_line(std::u8string_view line)
    {
        if (line.empty()) {
            return;
        }
        const Highlight_Type type = choose_line_highlight(line);
        emit_and_advance(line.length(), type);
    }
};

} // namespace
} // namespace diff

bool highlight_diff(
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    std::pmr::memory_resource*,
    const Highlight_Options& options
)
{
    return diff::Highlighter { out, source, options }();
}

} // namespace ulight
