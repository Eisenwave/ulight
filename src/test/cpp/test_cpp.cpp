#include <iostream>

#include <gtest/gtest.h>

#include "ulight/impl/platform.h"

#include "ulight/impl/lang/cpp.hpp"

namespace ulight::cpp {

ULIGHT_SUPPRESS_MISSING_DECLARATIONS_WARNING()

// NOLINTNEXTLINE(misc-use-internal-linkage)
std::ostream& operator<<(std::ostream& out, Escape_Type type)
{
    return out << int(type);
}

[[maybe_unused]]
std::ostream& operator<<(std::ostream& out, Escape_Result result) // NOLINT
{
    return out << "{ .length = " << result.length << ", .type = " << result.type << " }";
}

namespace {

TEST(Cpp, match_pp_number)
{
    EXPECT_EQ(match_pp_number(u8""), 0);
    EXPECT_EQ(match_pp_number(u8"0"), 1);

    EXPECT_EQ(match_pp_number(u8"100'000"), 7);
    EXPECT_EQ(match_pp_number(u8"0xff'ffuz"), 9);
    EXPECT_EQ(match_pp_number(u8"0xff'"), 4);

    EXPECT_EQ(match_pp_number(u8"0E"), 2);

    EXPECT_EQ(match_pp_number(u8"0E+"), 3);
    EXPECT_EQ(match_pp_number(u8"0e+"), 3);
    EXPECT_EQ(match_pp_number(u8"0P-"), 3);
    EXPECT_EQ(match_pp_number(u8"0p-"), 3);

    EXPECT_EQ(match_pp_number(u8"0e-3"), 4);
    EXPECT_EQ(match_pp_number(u8"0E+3"), 4);
}

TEST(Cpp, match_escape_sequence)
{
    EXPECT_EQ(match_escape_sequence(u8""), Escape_Result());
    EXPECT_EQ(match_escape_sequence(u8"x"), Escape_Result());

    EXPECT_EQ(match_escape_sequence(u8"\\'x"), Escape_Result(2, Escape_Type::simple));
    EXPECT_EQ(match_escape_sequence(u8"\\\"x"), Escape_Result(2, Escape_Type::simple));
    EXPECT_EQ(match_escape_sequence(u8"\\?x"), Escape_Result(2, Escape_Type::simple));
    EXPECT_EQ(match_escape_sequence(u8"\\ax"), Escape_Result(2, Escape_Type::simple));
    EXPECT_EQ(match_escape_sequence(u8"\\bx"), Escape_Result(2, Escape_Type::simple));
    EXPECT_EQ(match_escape_sequence(u8"\\fx"), Escape_Result(2, Escape_Type::simple));
    EXPECT_EQ(match_escape_sequence(u8"\\nx"), Escape_Result(2, Escape_Type::simple));
    EXPECT_EQ(match_escape_sequence(u8"\\rx"), Escape_Result(2, Escape_Type::simple));
    EXPECT_EQ(match_escape_sequence(u8"\\tx"), Escape_Result(2, Escape_Type::simple));
    EXPECT_EQ(match_escape_sequence(u8"\\vx"), Escape_Result(2, Escape_Type::simple));

    EXPECT_EQ(match_escape_sequence(u8"\\ \n\n"), Escape_Result(3, Escape_Type::newline));
    EXPECT_EQ(match_escape_sequence(u8"\\\t\n\n"), Escape_Result(3, Escape_Type::newline));
    EXPECT_EQ(match_escape_sequence(u8"\\\v\n\n"), Escape_Result(3, Escape_Type::newline));
    EXPECT_EQ(match_escape_sequence(u8"\\\f\n\n"), Escape_Result(3, Escape_Type::newline));
    EXPECT_EQ(match_escape_sequence(u8"\\\r\n\n"), Escape_Result(3, Escape_Type::newline));
    EXPECT_EQ(match_escape_sequence(u8"\\\n\n"), Escape_Result(2, Escape_Type::newline));

    EXPECT_EQ(match_escape_sequence(u8"\\u1234"), Escape_Result(6, Escape_Type::universal));
    EXPECT_EQ(match_escape_sequence(u8"\\u12345"), Escape_Result(6, Escape_Type::universal));

    EXPECT_EQ(match_escape_sequence(u8"\\U12345678"), Escape_Result(10, Escape_Type::universal));
    EXPECT_EQ(match_escape_sequence(u8"\\U123456789"), Escape_Result(10, Escape_Type::universal));

    EXPECT_EQ(match_escape_sequence(u8"\\u{}"), Escape_Result(4, Escape_Type::universal));
    EXPECT_EQ(match_escape_sequence(u8"\\u{0}"), Escape_Result(5, Escape_Type::universal));
    EXPECT_EQ(
        match_escape_sequence(u8"\\u{0123456789abcdef}"), Escape_Result(20, Escape_Type::universal)
    );

    EXPECT_EQ(match_escape_sequence(u8"\\N{}"), Escape_Result(4, Escape_Type::universal));
    EXPECT_EQ(match_escape_sequence(u8"\\N{DELETE}"), Escape_Result(10, Escape_Type::universal));
    EXPECT_EQ(
        match_escape_sequence(u8"\\N{EQUALS SIGN}"), Escape_Result(15, Escape_Type::universal)
    );

    EXPECT_EQ(match_escape_sequence(u8"\\x0x"), Escape_Result(3, Escape_Type::hexadecimal));
    EXPECT_EQ(match_escape_sequence(u8"\\xffffx"), Escape_Result(6, Escape_Type::hexadecimal));
    EXPECT_EQ(match_escape_sequence(u8"\\x{}"), Escape_Result(4, Escape_Type::hexadecimal));
    EXPECT_EQ(match_escape_sequence(u8"\\x{0}"), Escape_Result(5, Escape_Type::hexadecimal));
    EXPECT_EQ(
        match_escape_sequence(u8"\\x{0123456789abcdef}"),
        Escape_Result(20, Escape_Type::hexadecimal)
    );

    EXPECT_EQ(match_escape_sequence(u8"\\o{}"), Escape_Result(4, Escape_Type::octal));
    EXPECT_EQ(match_escape_sequence(u8"\\o{0}"), Escape_Result(5, Escape_Type::octal));
    EXPECT_EQ(match_escape_sequence(u8"\\o{01234567}"), Escape_Result(12, Escape_Type::octal));

    EXPECT_EQ(match_escape_sequence(u8"\\1x"), Escape_Result(2, Escape_Type::octal));
    EXPECT_EQ(match_escape_sequence(u8"\\12x"), Escape_Result(3, Escape_Type::octal));
    EXPECT_EQ(match_escape_sequence(u8"\\123x"), Escape_Result(4, Escape_Type::octal));
    EXPECT_EQ(match_escape_sequence(u8"\\1234x"), Escape_Result(4, Escape_Type::octal));

    EXPECT_EQ(match_escape_sequence(u8"\\$x"), Escape_Result(2, Escape_Type::conditional));
    EXPECT_EQ(match_escape_sequence(u8"\\#x"), Escape_Result(2, Escape_Type::conditional));
    EXPECT_EQ(match_escape_sequence(u8"\\#@"), Escape_Result(2, Escape_Type::conditional));
}

} // namespace
} // namespace ulight::cpp
