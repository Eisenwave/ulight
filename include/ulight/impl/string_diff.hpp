#ifndef ULIGHT_DIFF_HPP
#define ULIGHT_DIFF_HPP

#include "ulight/impl/platform.h"
#if ULIGHT_CPP // suppress unused include warning
#endif
#ifdef ULIGHT_EMSCRIPTEN
#error This header should not be included in Emscripten builds.
#endif

#include <algorithm>
#include <cstddef>
#include <ostream>
#include <span>
#include <string_view>
#include <vector>

#include "ulight/impl/ansi.hpp"
#include "ulight/impl/assert.hpp"
#include "ulight/impl/strings.hpp"

namespace ulight {

enum struct Edit_Type : signed char {
    /// @brief Delete an element in the source sequence.
    /// Advance by one element in the source sequence.
    del = -1,
    /// @brief Keep the element in the source sequence.
    /// Advance by one element in both sequences.
    common = 0,
    /// @brief Insert the element from the target sequence into the source sequence.
    /// Advance by one element in the target sequence.
    ins = 1,
};

/// @brief Uses the
/// [Needleman-Wunsch algorithm](https://en.wikipedia.org/wiki/Needleman%E2%80%93Wunsch_algorithm)
/// to compute the Shortest Edit Script to convert sequence `from` into sequence `to`.
inline std::vector<Edit_Type> shortest_edit_script(
    std::span<const std::u8string_view> from,
    std::span<const std::u8string_view> to
)
{
    const std::size_t f_data_size = (from.size() + 1) * (to.size() + 1);
    std::pmr::vector<std::size_t> f_data(f_data_size);
    const auto F = [&](std::size_t i, std::size_t j) -> std::size_t& {
        const std::size_t index = (i * (to.size() + 1)) + j;
        ULIGHT_DEBUG_ASSERT(index < f_data.size());
        return f_data[index];
    };

    for (std::size_t i = 0; i <= from.size(); ++i) {
        F(i, 0) = i;
    }
    for (std::size_t j = 0; j <= to.size(); ++j) {
        F(0, j) = j;
    }
    for (std::size_t i = 1; i <= from.size(); ++i) {
        for (std::size_t j = 1; j <= to.size(); ++j) {
            // The costs here are essentially the same as for Levenshtein distance computation,
            // except that if the two strings mismatch at a given index,
            // we consider the cost to be infinite.
            // This way, the output is made of pure insertions and deletions;
            // substitutions don't exist.
            F(i, j) = std::min({
                from[i - 1] == to[j - 1] ? F(i - 1, j - 1) : std::size_t(-1), // common
                F(i - 1, j) + 1, // deletion
                F(i, j - 1) + 1, // insertion
            });
        }
    }

    std::size_t i = from.size();
    std::size_t j = to.size();

    std::vector<Edit_Type> out;
    while (i != 0 || j != 0) {
        if (i != 0 && j != 0 && from[i - 1] == to[j - 1]) {
            out.push_back(Edit_Type::common);
            --i;
            --j;
        }
        else if (i != 0 && F(i, j) == F(i - 1, j) + 1) {
            out.push_back(Edit_Type::del);
            --i;
        }
        else {
            out.push_back(Edit_Type::ins);
            --j;
        }
    }

    std::ranges::reverse(out);
    auto it = out.begin();
    while (it != out.end()) {
        // Find the next block of insertions/deletions.
        const auto next_mod_begin = std::ranges::find_if(it, out.end(), [](Edit_Type t) {
            return t != Edit_Type::common;
        });
        const auto next_mod_end = std::ranges::find(next_mod_begin, out.end(), Edit_Type::common);
        // Partition the block so that deletions all precede insertions.
        std::ranges::partition(next_mod_begin, next_mod_end, [](Edit_Type t) {
            return t == Edit_Type::del;
        });
        it = next_mod_end;
    }

    return out;
}

inline void split_lines(std::vector<std::u8string_view>& out, std::u8string_view str)
{
    std::size_t pos = 0;
    std::size_t prev = 0;
    while ((pos = str.find('\n', prev)) != std::u8string_view::npos) {
        out.push_back(str.substr(prev, pos - prev));
        prev = pos + 1;
    }

    out.push_back(str.substr(prev));
}

inline void
print_diff_line(std::ostream& out, std::u8string_view line, std::string_view default_formatting)
{
    bool formatting_dirty = false;
    while (!line.empty()) {
        const std::size_t safe_length = line.find_first_of(u8"\t\r\v");
        if (safe_length != 0 && formatting_dirty) {
            out << default_formatting;
        }
        if (safe_length == std::u8string_view::npos) {
            out << as_string_view(line);
            return;
        }
        if (safe_length != 0) {
            out << as_string_view(line.substr(0, safe_length));
        }
        out << ansi::h_yellow;
        switch (line[safe_length]) {
        case u8'\t': out << "\\t"; break;
        case u8'\r': out << "\\r"; break;
        case u8'\v': out << "\\v"; break;
        default: ULIGHT_ASSERT_UNREACHABLE(u8"Unexpected non-safe character.");
        }
        formatting_dirty = true;
        line.remove_prefix(safe_length + 1);
    }
}

inline void print_diff(
    std::ostream& out,
    std::span<const std::u8string_view> from_lines,
    std::span<const std::u8string_view> to_lines
)
{
    const std::vector<Edit_Type> edits = shortest_edit_script(from_lines, to_lines);
    std::size_t from_index = 0;
    std::size_t to_index = 0;
    for (const auto e : edits) {
        switch (e) {
        case Edit_Type::common: {
            out << ansi::h_black << ' ';
            print_diff_line(out, from_lines[from_index], ansi::h_black);
            out << '\n';
            ++from_index;
            ++to_index;
            break;
        }
        case Edit_Type::del: {
            out << ansi::h_red << '-';
            print_diff_line(out, from_lines[from_index], ansi::h_red);
            out << '\n';
            ++from_index;
            break;
        }
        case Edit_Type::ins: {
            out << ansi::h_green << '+';
            print_diff_line(out, to_lines[to_index], ansi::h_green);
            out << '\n';
            ++to_index;
            break;
        }
        }
    }
}

inline void print_lines_diff(std::ostream& out, std::u8string_view from, std::u8string_view to)
{
    std::vector<std::u8string_view> from_lines;
    std::vector<std::u8string_view> to_lines;
    split_lines(from_lines, from);
    split_lines(to_lines, to);

    print_diff(out, from_lines, to_lines);
}

} // namespace ulight

#endif
