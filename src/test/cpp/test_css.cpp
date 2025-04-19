#include <iostream>

#include <gtest/gtest.h>

#include "ulight/impl/strings.hpp"

#include "ulight/impl/lang/css.hpp"

namespace ulight::css {
namespace {

[[maybe_unused]]
std::ostream& operator<<(std::ostream& out, Ident_Result result) // NOLINT
{
    return out << "{ .length = " << result.length
               << ", .type = " << as_string_view(enumerator_of(result.type)) << " }";
}

TEST(CSS, starts_with_number)
{
    // https://www.w3.org/TR/css-syntax-3/#starts-with-a-number
    EXPECT_FALSE(starts_with_number(u8""));
    EXPECT_FALSE(starts_with_number(u8"abc"));
    EXPECT_FALSE(starts_with_number(u8"##"));
    EXPECT_FALSE(starts_with_number(u8"abc123"));

    EXPECT_TRUE(starts_with_number(u8"123"));
    EXPECT_TRUE(starts_with_number(u8"-123"));
    EXPECT_TRUE(starts_with_number(u8".5"));
    EXPECT_TRUE(starts_with_number(u8"+5"));
    EXPECT_TRUE(starts_with_number(u8"-5"));
}

TEST(CSS, starts_with_valid_escape)
{
    // https://www.w3.org/TR/css-syntax-3/#starts-with-a-valid-escape
    EXPECT_FALSE(starts_with_valid_escape(u8""));
    EXPECT_FALSE(starts_with_valid_escape(u8"abc"));
    EXPECT_FALSE(starts_with_valid_escape(u8"\\"));
    EXPECT_FALSE(starts_with_valid_escape(u8"\\\n"));
    EXPECT_FALSE(starts_with_valid_escape(u8"\\\r"));
    EXPECT_FALSE(starts_with_valid_escape(u8"\\\r\n"));
    EXPECT_FALSE(starts_with_valid_escape(u8"\\\f"));

    EXPECT_TRUE(starts_with_valid_escape(u8"\\123"));
    EXPECT_TRUE(starts_with_valid_escape(u8"\\a"));
    EXPECT_TRUE(starts_with_valid_escape(u8"\\\\"));
    EXPECT_TRUE(starts_with_valid_escape(u8"\\ "));
}

TEST(CSS, starts_with_ident_sequence)
{
    // https://www.w3.org/TR/css-syntax-3/#would-start-an-identifier
    EXPECT_FALSE(starts_with_ident_sequence(u8""));
    EXPECT_FALSE(starts_with_ident_sequence(u8"\\"));
    EXPECT_FALSE(starts_with_ident_sequence(u8"\\\n"));
    EXPECT_FALSE(starts_with_ident_sequence(u8"\\\r"));
    EXPECT_FALSE(starts_with_ident_sequence(u8"\\\r\n"));
    EXPECT_FALSE(starts_with_ident_sequence(u8"\\\f"));
    EXPECT_FALSE(starts_with_ident_sequence(u8"-0"));

    EXPECT_TRUE(starts_with_ident_sequence(u8"\\123"));
    EXPECT_TRUE(starts_with_ident_sequence(u8"\\a"));
    EXPECT_TRUE(starts_with_ident_sequence(u8"\\\\"));
    EXPECT_TRUE(starts_with_ident_sequence(u8"\\ "));
    EXPECT_TRUE(starts_with_ident_sequence(u8"abc"));
    EXPECT_TRUE(starts_with_ident_sequence(u8"_"));
    EXPECT_TRUE(starts_with_ident_sequence(u8"-a"));
    EXPECT_TRUE(starts_with_ident_sequence(u8"-\\ "));
}

TEST(CSS, match_number)
{
    // https://www.w3.org/TR/css-syntax-3/#consume-number
    EXPECT_EQ(match_number(u8""), 0);
    EXPECT_EQ(match_number(u8"x"), 0);
    EXPECT_EQ(match_number(u8"x12"), 0);

    EXPECT_EQ(match_number(u8"123"), 3);
    EXPECT_EQ(match_number(u8"123abc"), 3);
    EXPECT_EQ(match_number(u8".123"), 4);
    EXPECT_EQ(match_number(u8"+123"), 4);
    EXPECT_EQ(match_number(u8"-123"), 4);

    EXPECT_EQ(match_number(u8"+123E"), 4);
    EXPECT_EQ(match_number(u8"+123E+"), 4);
    EXPECT_EQ(match_number(u8"+123E-"), 4);
    EXPECT_EQ(match_number(u8"-123E5"), 4);
    EXPECT_EQ(match_number(u8"-123E+5"), 7);
    EXPECT_EQ(match_number(u8"-123E-5"), 7);
}

TEST(CSS, match_escaped_code_point)
{
    // https://www.w3.org/TR/css-syntax-3/#consume-escaped-code-point
    EXPECT_EQ(match_escaped_code_point(u8""), 0);
    EXPECT_EQ(match_escaped_code_point(u8"a"), 1);

    EXPECT_EQ(match_escaped_code_point(u8"1"), 1);
    EXPECT_EQ(match_escaped_code_point(u8"12"), 2);
    EXPECT_EQ(match_escaped_code_point(u8"123"), 3);
    EXPECT_EQ(match_escaped_code_point(u8"1234"), 4);
    EXPECT_EQ(match_escaped_code_point(u8"12345"), 5);
    EXPECT_EQ(match_escaped_code_point(u8"123456"), 6);
    EXPECT_EQ(match_escaped_code_point(u8"1234567"), 6);
    EXPECT_EQ(match_escaped_code_point(u8"abcdef7"), 6);

    EXPECT_EQ(match_escaped_code_point(u8"123 456"), 4);

    EXPECT_EQ(match_escaped_code_point(u8"\\"), 1);
    EXPECT_EQ(match_escaped_code_point(u8"."), 1);
    EXPECT_EQ(match_escaped_code_point(u8"?"), 1);

    constexpr std::u8string_view umlaut = u8"ä";
    EXPECT_EQ(match_escaped_code_point(umlaut), umlaut.length());
    constexpr std::u8string_view umlaut_x = u8"äx";
    EXPECT_EQ(match_escaped_code_point(umlaut_x), umlaut.length());
}

TEST(CSS, match_ident_sequence)
{
    // https://www.w3.org/TR/css-syntax-3/#consume-name
    EXPECT_EQ(match_ident_sequence(u8""), 0);

    EXPECT_EQ(match_ident_sequence(u8"123"), 3);
    EXPECT_EQ(match_ident_sequence(u8"abc"), 3);
    EXPECT_EQ(match_ident_sequence(u8"abc\\x"), 5);
    EXPECT_EQ(match_ident_sequence(u8"\\xabc"), 5);

    EXPECT_EQ(match_ident_sequence(u8"abc;def"), 3);
}

} // namespace
} // namespace ulight::css
