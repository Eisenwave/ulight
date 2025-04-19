#ifndef ULIGHT_DIFF_HPP
#define ULIGHT_DIFF_HPP

#include <string_view>

#include "ulight/ulight.hpp"

namespace ulight::diff {

[[nodiscard]]
Highlight_Type choose_line_highlight(std::u8string_view line);

}

#endif
