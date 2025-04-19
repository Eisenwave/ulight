#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "ulight/impl/ansi.hpp"
#include "ulight/impl/io.hpp"
#include "ulight/impl/string_diff.hpp"
#include "ulight/impl/strings.hpp"
#include "ulight/ulight.hpp"

namespace ulight {
namespace {

namespace fs = std::filesystem;
using ulight::as_string_view;

[[nodiscard]]
std::u8string_view as_string_view(std::span<const char8_t> span)
{
    return { span.data(), span.size() };
}

struct Highlight_Test : testing::Test {
    std::vector<char8_t> actual;
    std::vector<char8_t> expected;
    std::vector<char8_t> source;

    [[nodiscard]]
    bool load_code(const fs::path& path)
    {
        return load_utf8_file_or_error(source, path.c_str());
    }

    [[nodiscard]]
    bool load_expectations(const fs::path& path)
    {
        return load_utf8_file_or_error(expected, path.c_str());
    }

    void clear()
    {
        actual.clear();
        source.clear();
        expected.clear();
    }
};

[[nodiscard]]
std::vector<fs::path> paths_in_directory(const fs::path& directory)
{
    constexpr auto is_regular_file
        = [](const fs::directory_entry& entry) { return entry.is_regular_file(); };
    constexpr auto to_path = [](const fs::directory_entry& entry) -> auto& { return entry.path(); };

    // TODO: replace with std::from_range once supported
    fs::recursive_directory_iterator iterator(directory);
    std::vector<fs::path> result;
    std::ranges::copy(
        iterator | std::views::filter(is_regular_file) | std::views::transform(to_path),
        std::back_inserter(result)
    );
    return result;
}

TEST_F(Highlight_Test, file_tests)
{
    Token token_buffer[4096];
    char text_buffer[8192];

    static const fs::path directory { "test/highlight" };
    ASSERT_TRUE(fs::is_directory(directory));

    std::vector<fs::path> paths = paths_in_directory(directory);
    std::ranges::sort(paths);

    for (const auto& input_path : paths) {
        const std::u8string extension = input_path.extension().generic_u8string();
        ASSERT_TRUE(extension.size() > 1);

        fs::path expectations_path = input_path;
        expectations_path += ".html";
        const bool has_expectations = fs::is_regular_file(expectations_path);

        const std::u8string_view lang_name = std::u8string_view(extension).substr(1);
        const Lang lang = get_lang(lang_name);
        EXPECT_TRUE(lang != Lang::none);
        if (lang == Lang::none) {
            std::cout << ansi::h_red << "BAD LANG: " //
                      << ansi::reset << input_path //
                      << ansi::h_black << " (" << as_string_view(lang_name) << ")\n";
            continue;
        }

        clear();

        if (!load_code(input_path) || (has_expectations && !load_expectations(expectations_path))) {
            continue;
        }

        auto flush_buffer = [&](const char* text, std::size_t length) {
            const std::u8string_view sv { reinterpret_cast<const char8_t*>(text), length };
            actual.insert(actual.end(), sv.begin(), sv.end());
        };

        State state;
        state.set_source(as_string_view(source));
        state.set_lang(lang);
        state.set_token_buffer(token_buffer);
        state.set_text_buffer(text_buffer);
        state.on_flush_text(flush_buffer);

        const auto pretty_print_error = [&] {
            std::cout << ansi::h_red << "ERROR: " //
                      << ansi::reset << input_path //
                      << ansi::h_black << " (" << state.get_error_string() << ")\n";
        };

        const Status status = state.source_to_html();
        EXPECT_EQ(status, Status::ok);
        if (status != Status::ok) {
            pretty_print_error();
            continue;
        }

        const bool compare_succeeded = !has_expectations || expected == actual;
        if (!compare_succeeded) {
            std::cout << ansi::h_red << "FAIL: " //
                      << ansi::reset << input_path //
                      << ":\nActual (" << input_path << ") -> expected (" << expectations_path
                      << ") difference:\n";
            print_lines_diff(std::cout, as_string_view(actual), as_string_view(expected));
            std::cout << ansi::reset << '\n';
        }
        EXPECT_TRUE(compare_succeeded);

        std::cout << ansi::h_green << "OK: " //
                  << ansi::reset << input_path.generic_string();
        if (!has_expectations && extension != u8".html") {
            std::cout << ansi::h_yellow << " (no expectations)" //
                      << ansi::reset;
        }
        std::cout << '\n';
    }
}

[[nodiscard]]
std::string_view lang_display_name(ulight_lang lang)
{
    const auto i = std::size_t(lang);
    return { ulight_lang_display_names[i].text, ulight_lang_display_names[i].length };
}

TEST_F(Highlight_Test, exhaustive_one_char)
{
    Token token_buffer[16];
    char8_t source;

    bool success = true;
    for (int i = 1; i < int(ULIGHT_LANG_COUNT); ++i) {
        bool lang_success = true;
        const auto lang = ulight_lang(i);
        const std::string_view source_view { reinterpret_cast<const char*>(&source), 1 };

        State state;
        state.set_source(source_view);
        state.set_lang(lang);
        state.set_token_buffer(token_buffer);
        state.on_flush_tokens(Constant<[](Token* tokens, std::size_t amount) {
            ULIGHT_ASSERT(amount <= 1);
            if (amount != 0) {
                ULIGHT_ASSERT(tokens != nullptr);
                ULIGHT_ASSERT(tokens->begin == 0);
                ULIGHT_ASSERT(tokens->length == 1);
            }
        }> {});

        for (char8_t i = 0; i <= u8'\u007F'; ++i) {
            source = i;
            const Status status = state.source_to_tokens();
            if (status != Status::ok && status != Status::bad_code) {
                lang_success = false;
                std::cout << ansi::h_red << "FAIL: " << ansi::reset << lang_display_name(lang) //
                          << ", for" << " U+00" << std::hex << int(source) << std::dec;
                if (!is_html_ascii_control(source)) {
                    std::cout << " '" << source_view << '\'';
                }
                std::cout << ": " << state.get_error_string() << "\n";
            }
        }

        if (lang_success) {
            std::cout << ansi::h_green << "OK: " << ansi::reset << lang_display_name(lang) << '\n';
        }
        else {
            success = false;
        }
    }
    ASSERT_TRUE(success);
}

TEST_F(Highlight_Test, exhaustive_three_chars)
{
    Token token_buffer[16];
    char8_t source[3];

    bool success = true;
    for (int i = 1; i < int(ULIGHT_LANG_COUNT); ++i) {
        bool lang_success = true;
        const auto lang = ulight_lang(i);
        const std::string_view source_view { reinterpret_cast<const char*>(&source),
                                             std::size(source) };

        State state;
        state.set_source(source_view);
        state.set_lang(lang);
        state.set_token_buffer(token_buffer);
        state.on_flush_tokens(Constant<[](Token*, std::size_t amount) { //
            ULIGHT_ASSERT(amount <= sizeof(source));
        }> {});

        for (std::size_t i = 0; i < 1uz << 21; ++i) {
            source[0] = (i >> 0) & 0x7f;
            source[1] = (i >> 7) & 0x7f;
            source[2] = (i >> 14) & 0x7f;

            const Status status = state.source_to_tokens();
            if (status != Status::ok && status != Status::bad_code) {
                lang_success = false;
                std::cout << ansi::h_red << "FAIL: " //
                          << ansi::reset << lang_display_name(lang) //
                          << ": " << state.get_error_string() << "\n";
                break;
            }
        }

        if (lang_success) {
            std::cout << ansi::h_green << "OK: " << ansi::reset << lang_display_name(lang) << '\n';
        }
        else {
            success = false;
        }
    }
    ASSERT_TRUE(success);
}

} // namespace
} // namespace ulight
