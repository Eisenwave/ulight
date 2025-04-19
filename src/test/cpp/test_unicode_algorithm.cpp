#include <gtest/gtest.h>

#include "ulight/impl/unicode_algorithm.hpp"

namespace ulight::utf8 {
namespace {

// TODO: create non-ASCII tests too

TEST(Unicode_Algorithm, find_if_ascii)
{
    EXPECT_EQ(find_if(u8"abc", [](char32_t c) { return c == U'a'; }), 0);
    EXPECT_EQ(find_if(u8"abc", [](char32_t c) { return c == U'b'; }), 1);
    EXPECT_EQ(find_if(u8"abc", [](char32_t c) { return c == U'c'; }), 2);
    EXPECT_EQ(find_if(u8"abc", [](char32_t c) { return c == U'd'; }), std::u8string_view::npos);
}

TEST(Unicode_Algorithm, find_if_not_ascii)
{
    EXPECT_EQ(find_if_not(u8"abc", [](char32_t c) { return c == U'a'; }), 1);
    EXPECT_EQ(find_if_not(u8"abc", [](char32_t c) { return c == U'b'; }), 0);
    EXPECT_EQ(find_if_not(u8"abc", [](char32_t c) { return c == U'c'; }), 0);
    EXPECT_EQ(find_if_not(u8"abc", [](char32_t c) { return c != U'd'; }), std::u8string_view::npos);
}

TEST(Unicode_Algorithm, length_if_ascii)
{
    EXPECT_EQ(length_if(u8"abc", [](char32_t c) { return c == U'a'; }), 1);
    EXPECT_EQ(length_if(u8"abc", [](char32_t c) { return c == U'b'; }), 0);
    EXPECT_EQ(length_if(u8"abc", [](char32_t c) { return c == U'c'; }), 0);
    EXPECT_EQ(length_if(u8"abc", [](char32_t c) { return c != U'd'; }), 3);
}

TEST(Unicode_Algorithm, length_if_not_ascii)
{
    EXPECT_EQ(length_if_not(u8"abc", [](char32_t c) { return c == U'a'; }), 0);
    EXPECT_EQ(length_if_not(u8"abc", [](char32_t c) { return c == U'b'; }), 1);
    EXPECT_EQ(length_if_not(u8"abc", [](char32_t c) { return c == U'c'; }), 2);
    EXPECT_EQ(length_if_not(u8"abc", [](char32_t c) { return c == U'd'; }), 3);
}

} // namespace
} // namespace ulight::utf8
