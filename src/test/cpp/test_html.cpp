#include <iostream>

#include <gtest/gtest.h>

#include "ulight/impl/lang/html.hpp"

namespace ulight::html {
namespace {

[[maybe_unused]]
std::ostream& operator<<(std::ostream& out, Match_Result result) // NOLINT
{
    return out << "{ .length = " << result.length
               << ", .terminated = " << (result.terminated ? "true" : "false") << " }";
}

TEST(HTML, match_whitespace)
{
    EXPECT_EQ(match_whitespace(u8""), 0);
    EXPECT_EQ(match_whitespace(u8"  "), 2);
    EXPECT_EQ(match_whitespace(u8"  \n"), 3);
    EXPECT_EQ(match_whitespace(u8"s  \n"), 0);
}

TEST(HTML, match_character_reference)
{
    EXPECT_EQ(match_character_reference(u8""), 0);
    EXPECT_EQ(match_character_reference(u8" &s;"), 0);
    EXPECT_EQ(match_character_reference(u8"x&s;"), 0);
    EXPECT_EQ(match_character_reference(u8"&"), 0);
    EXPECT_EQ(match_character_reference(u8"&;"), 0);
    EXPECT_EQ(match_character_reference(u8"&#;"), 0);
    EXPECT_EQ(match_character_reference(u8"&#x;"), 0);

    EXPECT_EQ(match_character_reference(u8"&s;"), 3);
    EXPECT_EQ(match_character_reference(u8"&s;abc"), 3);
    EXPECT_EQ(match_character_reference(u8"&abc;"), 5);
    EXPECT_EQ(match_character_reference(u8"&#12345;"), 8);
    EXPECT_EQ(match_character_reference(u8"&#12345f;"), 0);

    EXPECT_EQ(match_character_reference(u8"&#x12345f;"), 10);
}

TEST(HTML, match_comment)
{
    EXPECT_EQ(match_comment(u8""), Match_Result());
    EXPECT_EQ(match_comment(u8"<tag></tag>"), Match_Result());
    EXPECT_EQ(match_comment(u8"<!"), Match_Result());
    EXPECT_EQ(match_comment(u8"<!-"), Match_Result());
    EXPECT_EQ(match_comment(u8"<!-->"), Match_Result());
    EXPECT_EQ(match_comment(u8"<!--->"), Match_Result());

    EXPECT_EQ(match_comment(u8"<!--"), Match_Result(4, false));

    EXPECT_EQ(match_comment(u8"<!---->"), Match_Result(7, true));
    EXPECT_EQ(match_comment(u8"<!--<!-->"), Match_Result(9, true));
}

TEST(HTML, match_doctype_permissive)
{
    EXPECT_EQ(match_doctype_permissive(u8""), Match_Result());
    EXPECT_EQ(match_doctype_permissive(u8"<!"), Match_Result());

    EXPECT_EQ(match_doctype_permissive(u8"<!DOCTYPE"), Match_Result(9, false));
    EXPECT_EQ(match_doctype_permissive(u8"<!DOCTYPE>"), Match_Result(10, true));
    EXPECT_EQ(match_doctype_permissive(u8"<!DOCTYPE>abc"), Match_Result(10, true));
    EXPECT_EQ(match_doctype_permissive(u8"<!DOCTYPE html"), Match_Result(14, false));
    EXPECT_EQ(match_doctype_permissive(u8"<!DOCTYPE html>"), Match_Result(15, true));
    EXPECT_EQ(match_doctype_permissive(u8"<!DOCTYPE   html>"), Match_Result(17, true));
}

TEST(HTML, match_cdata)
{
    EXPECT_EQ(match_cdata(u8""), Match_Result());
    EXPECT_EQ(match_cdata(u8"<!"), Match_Result());
    EXPECT_EQ(match_cdata(u8"<!["), Match_Result());
    EXPECT_EQ(match_cdata(u8"<![CDATA"), Match_Result());

    EXPECT_EQ(match_cdata(u8"<![CDATA["), Match_Result(9, false));
    EXPECT_EQ(match_cdata(u8"<![CDATA[>"), Match_Result(10, false));
    EXPECT_EQ(match_cdata(u8"<![CDATA[]>"), Match_Result(11, false));

    EXPECT_EQ(match_cdata(u8"<![CDATA[]]>"), Match_Result(12, true));
    EXPECT_EQ(match_cdata(u8"<![CDATA[]]>abc"), Match_Result(12, true));
    EXPECT_EQ(match_cdata(u8"<![CDATA[raw data]]>"), Match_Result(20, true));
}

TEST(HTML, match_raw_text)
{
    constexpr auto match = [](std::u8string_view str) { //
        return match_raw_text(str, u8"script");
    };

    EXPECT_EQ(match(u8""), 0);
    EXPECT_EQ(match(u8"abc"), 3);
    EXPECT_EQ(match(u8"abc<"), 4);
    EXPECT_EQ(match(u8"abc</"), 5);
    EXPECT_EQ(match(u8"abc</tag>"), 9);
    EXPECT_EQ(match(u8"abc</script"), 11);

    EXPECT_EQ(match(u8"abc</script>"), 3);
    EXPECT_EQ(match(u8"abc</script "), 3);
    EXPECT_EQ(match(u8"abc</script\t"), 3);
    EXPECT_EQ(match(u8"abc</script\n"), 3);
    EXPECT_EQ(match(u8"abc</script\r"), 3);
    EXPECT_EQ(match(u8"abc</script\f"), 3);

    EXPECT_EQ(match(u8"<script>abc</script>"), 11);
}

TEST(HTML, match_escapable_raw_text)
{
    constexpr auto match = [](std::u8string_view str) { //
        return match_escapable_raw_text_piece(str, u8"script");
    };

    EXPECT_EQ(match(u8""), Raw_Text_Result());
    EXPECT_EQ(match(u8"abc"), Raw_Text_Result(3, 0));
    EXPECT_EQ(match(u8"abc<"), Raw_Text_Result(4, 0));
    EXPECT_EQ(match(u8"abc</"), Raw_Text_Result(5, 0));
    EXPECT_EQ(match(u8"abc</tag>"), Raw_Text_Result(9, 0));
    EXPECT_EQ(match(u8"abc</script"), Raw_Text_Result(11, 0));

    EXPECT_EQ(match(u8"abc</script>"), Raw_Text_Result(3, 0));
    EXPECT_EQ(match(u8"abc</script "), Raw_Text_Result(3, 0));
    EXPECT_EQ(match(u8"abc</script\t"), Raw_Text_Result(3, 0));
    EXPECT_EQ(match(u8"abc</script\n"), Raw_Text_Result(3, 0));
    EXPECT_EQ(match(u8"abc</script\r"), Raw_Text_Result(3, 0));
    EXPECT_EQ(match(u8"abc</script\f"), Raw_Text_Result(3, 0));

    EXPECT_EQ(match(u8"<script>abc</script>"), Raw_Text_Result(11, 0));

    EXPECT_EQ(match(u8"abc&;</script>"), Raw_Text_Result(5, 0));
    EXPECT_EQ(match(u8"abc&s;</script>"), Raw_Text_Result(3, 3));
    EXPECT_EQ(match(u8"&s;</script>"), Raw_Text_Result(0, 3));
    EXPECT_EQ(match(u8"&s;abc</script>"), Raw_Text_Result(0, 3));
}

TEST(HTML, match_end_tag_permissive)
{
    EXPECT_EQ(match_end_tag_permissive(u8""), End_Tag_Result());
    EXPECT_EQ(match_end_tag_permissive(u8"</>"), End_Tag_Result());
    EXPECT_EQ(match_end_tag_permissive(u8"</b"), End_Tag_Result());
    EXPECT_EQ(match_end_tag_permissive(u8"</b "), End_Tag_Result());
    EXPECT_EQ(match_end_tag_permissive(u8"a</b>"), End_Tag_Result());

    EXPECT_EQ(match_end_tag_permissive(u8"</b>"), End_Tag_Result(4, 1));
    EXPECT_EQ(match_end_tag_permissive(u8"</script>"), End_Tag_Result(9, 6));
}

TEST(HTML, match_tag_name)
{
    EXPECT_EQ(match_tag_name(u8""), 0);
    EXPECT_EQ(match_tag_name(u8"<"), 0);
    EXPECT_EQ(match_tag_name(u8"<abc"), 0);

    EXPECT_EQ(match_tag_name(u8"a"), 1);
    EXPECT_EQ(match_tag_name(u8"abc"), 3);
    EXPECT_EQ(match_tag_name(u8"abc>"), 3);
}

TEST(HTML, match_attribute_name)
{
    EXPECT_EQ(match_attribute_name(u8""), 0);
    EXPECT_EQ(match_attribute_name(u8">"), 0);
    EXPECT_EQ(match_attribute_name(u8">abc"), 0);

    EXPECT_EQ(match_attribute_name(u8"a"), 1);
    EXPECT_EQ(match_attribute_name(u8"abc"), 3);
    EXPECT_EQ(match_attribute_name(u8"abc>"), 3);
    EXPECT_EQ(match_attribute_name(u8"<"), 1);
    EXPECT_EQ(match_attribute_name(u8"<abc"), 4);
}

} // namespace
} // namespace ulight::html
