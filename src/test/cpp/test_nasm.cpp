#include <gtest/gtest.h>
#include "ulight/impl/nasm.hpp"
#include <string_view>


namespace ulight::nasm{

namespace{

TEST(NASM, line_comment)
{

    const std::u8string_view comment[]{
        u8"; this is a nasm comment",
        u8";this too is a nasm comment",
        u8";_____comment_____",
        u8";        ",
        u8";",
        u8"  ",
        u8"this is not a nasm comment"
    };

    EXPECT_EQ(line_comment(comment[0]), comment[0].size());
    EXPECT_EQ(line_comment(comment[1]), comment[1].size());
    EXPECT_EQ(line_comment(comment[2]), comment[2].size());
    EXPECT_EQ(line_comment(comment[3]), comment[3].size());
    EXPECT_EQ(line_comment(comment[4]), comment[4].size());
    EXPECT_EQ(line_comment(comment[5]), 0);
    EXPECT_EQ(line_comment(comment[6]), 0);
}



TEST(NASM, line_label)
{

    const std::u8string_view label[]{
        u8"nasm_label:",
        u8".another_nasm_label:",
        u8"__yet_another_nasm_label:",
        u8".is_this_a_nasm_label??:",
        u8"..@nasm_label2:",
        u8"@@test_nasm_label:",
        u8"?#this_is_also_label:",
        u8"#not_a_nasm_label:",
        u8".invalid:_nasm_label:",
        u8"..anotherinvalid:_nasm_label",
        u8".@wrong=_label:"
    };

    EXPECT_EQ(line_label(label[0]), label[0].size());
    EXPECT_EQ(line_label(label[1]), label[1].size());
    EXPECT_EQ(line_label(label[2]), label[2].size());
    EXPECT_EQ(line_label(label[3]), label[3].size());
    EXPECT_EQ(line_label(label[4]), label[4].size());
    EXPECT_EQ(line_label(label[5]), 0);
    EXPECT_EQ(line_label(label[6]), label[6].size());
    EXPECT_EQ(line_label(label[7]), 0);
    EXPECT_EQ(line_label(label[8]), 0);
    EXPECT_EQ(line_label(label[9]), 0);
    EXPECT_EQ(line_label(label[10]), 0);

}







} // anonymous namespace
} // nasm namespace
