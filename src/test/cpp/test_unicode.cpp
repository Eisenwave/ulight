#include <algorithm>
#include <cstdint>
#include <expected>
#include <iterator>
#include <memory_resource>
#include <random>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "ulight/impl/io.hpp"
#include "ulight/impl/unicode.hpp"
#include "ulight/impl/unicode_chars.hpp"

namespace ulight::utf8 {
namespace {

[[nodiscard]]
std::pmr::vector<char32_t> to_utf32(std::u8string_view utf8, std::pmr::memory_resource* memory)
{
    std::pmr::vector<char32_t> result { memory };
    std::ranges::copy(Code_Point_View { utf8 }, std::back_inserter(result));
    return result;
}

TEST(Unicode, sequence_length)
{
    // https://en.wikipedia.org/wiki/UTF-8
    EXPECT_EQ(sequence_length(0b0000'0000), 1);
    EXPECT_EQ(sequence_length(0b1000'0000), 0);
    EXPECT_EQ(sequence_length(0b1100'0000), 2);
    EXPECT_EQ(sequence_length(0b1110'0000), 3);
    EXPECT_EQ(sequence_length(0b1111'0000), 4);
    EXPECT_EQ(sequence_length(0b1111'1000), 0);
}

TEST(Unicode, decode_unchecked)
{
    EXPECT_EQ(decode_unchecked(u8"a"), U'a');
    EXPECT_EQ(decode_unchecked(u8"\u00E9"), U'\u00E9');
    EXPECT_EQ(decode_unchecked(u8"\u0905"), U'\u0905');
    EXPECT_EQ(decode_unchecked(u8"\U0001F600"), U'\U0001F600');
}

TEST(Unicode, decode_file)
{
    std::pmr::monotonic_buffer_resource memory;

    const std::expected<std::vector<char8_t>, IO_Error_Code> utf8 = load_utf8_file("test/utf8.txt");
    ASSERT_TRUE(utf8);

    const std::expected<std::vector<char32_t>, IO_Error_Code> expected
        = load_utf32le_file("test/utf32le.txt");
    ASSERT_TRUE(expected);

    const std::u8string_view u8view { utf8->data(), utf8->size() };

    const std::pmr::vector<char32_t> actual = to_utf32(u8view, &memory);

    ASSERT_TRUE(std::ranges::equal(*expected, actual));
}

TEST(Unicode, encode_decode_reversible_fuzzing)
{
    constexpr int iterations = 1'000'000;

    std::default_random_engine rng { 12345 };
    std::uniform_int_distribution<std::uint32_t> distr { 0, code_point_max };

    for (int i = 0; i < iterations; ++i) {
        const char32_t code_point = distr(rng);
        if (!is_scalar_value(code_point)) {
            continue;
        }

        const Code_Units_And_Length encoded = encode8_unchecked(code_point);
        const std::expected<Code_Point_And_Length, Error_Code> decoded
            = decode_and_length(encoded.as_string());
        ASSERT_TRUE(decoded);
        EXPECT_EQ(decoded->length, encoded.length);
        EXPECT_EQ(decoded->code_point, code_point);
    }
}

} // namespace
} // namespace ulight::utf8
