#include <cstddef>
#include <expected>
#include <memory_resource>
#include <string_view>

#include "ulight/ulight.hpp"

#include "ulight/impl/assert.hpp"
#include "ulight/impl/buffer.hpp"
#include "ulight/impl/highlight.hpp"
#include "ulight/impl/highlighter.hpp"
#include "ulight/impl/unicode.hpp"
#include "ulight/impl/unicode_algorithm.hpp"

#include "ulight/impl/lang/mmml.hpp"
#include "ulight/impl/lang/mmml_chars.hpp"

namespace ulight {
namespace mmml {

std::size_t match_directive_name(std::u8string_view str)
{
    constexpr auto predicate = [](char32_t c) { return is_mmml_directive_name(c); };
    return str.empty() || is_ascii_digit(str[0]) ? 0 : utf8::length_if(str, predicate);
}

std::size_t match_argument_name(std::u8string_view str)
{
    constexpr auto predicate = [](char32_t c) { return is_mmml_argument_name_character(c); };
    return str.empty() || is_ascii_digit(str[0]) ? 0 : utf8::length_if(str, predicate);
}

std::size_t match_whitespace(std::u8string_view str)
{
    constexpr auto predicate = [](char8_t c) { return is_html_whitespace(c); };
    const auto* const data_end = str.data() + str.size();
    const auto* const end = std::ranges::find_if_not(str.begin(), data_end, predicate);
    return std::size_t(data_end - end);
}

bool starts_with_escape_or_directive(std::u8string_view str)
{
    if (str.length() < 2 || str[0] != u8'\\') {
        return false;
    }
    if (is_mmml_escapeable(str[1])) {
        return true;
    }
    const auto [next_point, _] = utf8::decode_and_length_or_throw(str.substr(1));
    return is_mmml_directive_name_start(next_point);
}

Named_Argument_Result match_named_argument_prefix(const std::u8string_view str)
{
    std::size_t length = 0;
    const std::size_t leading_whitespace = match_whitespace(str);
    length += leading_whitespace;
    if (length >= str.length()) {
        return {};
    }

    const std::size_t name_length = match_argument_name(str.substr(length));
    if (name_length == 0 || length >= str.length()) {
        return {};
    }
    length += name_length;

    const std::size_t trailing_whitespace = match_whitespace(str.substr(length));
    length += trailing_whitespace;
    if (length >= str.length()) {
        return {};
    }
    if (str[length] != u8'=') {
        return {};
    }
    ++length;
    ULIGHT_ASSERT(length == leading_whitespace + name_length + trailing_whitespace + 1);

    return { .length = length,
             .leading_whitespace = leading_whitespace,
             .name_length = name_length,
             .trailing_whitespace = trailing_whitespace };
}

namespace {

struct Bracket_Levels {
    std::size_t square = 0;
    std::size_t brace = 0;
};

enum struct Content_Context : Underlying {
    /// @brief The whole document.
    document,
    /// @brief A single argument within `[...]`.
    argument_value,
    /// @brief `{...}`.
    block
};

[[nodiscard]]
bool is_terminated_by(Content_Context context, char8_t c)
{
    switch (context) {
    case Content_Context::argument_value: //
        return c == u8',' || c == u8']' || c == u8'}';
    case Content_Context::block: //
        return c == u8'}';
    default: //
        return false;
    }
}

struct Consumer {
    virtual void whitespace(std::size_t length) = 0;
    virtual void text(std::size_t length) = 0;
    virtual void opening_square() = 0;
    virtual void closing_square() = 0;
    virtual void comma() = 0;
    virtual void argument_name(std::size_t length) = 0;
    virtual void equals() = 0;
    virtual void directive_name(std::size_t length) = 0;
    virtual void opening_brace() = 0;
    virtual void closing_brace() = 0;
    virtual void escape() = 0;

    virtual void push_directive() { }
    virtual void pop_directive() { }
    virtual void unexpected_eof() { }
};

[[nodiscard]]
std::size_t match_escape(Consumer& out, const std::u8string_view str)
{
    constexpr std::size_t sequence_length = 2;
    if (str.length() < sequence_length || str[0] != u8'\\' || !is_mmml_escapeable(str[1])) {
        return 0;
    }
    out.escape();
    return sequence_length;
}

std::size_t match_directive(Consumer& out, std::u8string_view str);

std::size_t match_content(
    Consumer& out,
    std::u8string_view str,
    Content_Context context,
    Bracket_Levels& levels
)
{
    if (const std::size_t e = match_escape(out, str)) {
        return e;
    }
    if (const std::size_t d = match_directive(out, str)) {
        return d;
    }
    std::size_t plain_length = 0;

    for (; plain_length < str.length(); ++plain_length) {
        const char8_t c = str[plain_length];
        if (c == u8'\\') {
            if (starts_with_escape_or_directive(str.substr(plain_length))) {
                break;
            }
            continue;
        }
        if (context == Content_Context::document) {
            continue;
        }
        if (context == Content_Context::argument_value) {
            if (c == u8',') {
                break;
            }
            if (c == u8'[') {
                ++levels.square;
            }
            if (c == u8']' && levels.square-- == 0) {
                break;
            }
        }
        if (c == u8'{') {
            ++levels.brace;
        }
        if (c == u8'}' && levels.brace-- == 0) {
            break;
        }
    }

    out.text(plain_length);
    return plain_length;
}

std::size_t match_content_sequence(Consumer& out, std::u8string_view str, Content_Context context)
{
    Bracket_Levels levels {};
    std::size_t length = 0;

    while (!str.empty() && !is_terminated_by(context, str[0])) {
        const std::size_t content_length = match_content(out, str, context, levels);
        ULIGHT_ASSERT(content_length != 0);
        str.remove_prefix(content_length);
        length += content_length;
    }
    return length;
}

std::size_t match_argument(Consumer& out, std::u8string_view str)
{
    Named_Argument_Result name = match_named_argument_prefix(str);
    if (name) {
        if (name.leading_whitespace) {
            out.whitespace(name.leading_whitespace);
        }
        out.argument_name(name.name_length);
        if (name.trailing_whitespace) {
            out.whitespace(name.trailing_whitespace);
        }
        out.equals();
    }
    const std::size_t content_length
        = match_content_sequence(out, str.substr(name.length), Content_Context::argument_value);
    return name.length + content_length;
}

std::size_t match_argument_list(Consumer& out, std::u8string_view str)
{
    if (!str.starts_with(u8'[')) {
        return 0;
    }
    out.opening_square();
    str.remove_prefix(1);

    std::size_t length = 1;
    while (!str.empty()) {
        const std::size_t arg_length = match_argument(out, str);
        length += arg_length;
        str.remove_prefix(arg_length);

        if (str.empty()) {
            break;
        }
        if (str[0] == u8'}') {
            break;
        }
        if (str[0] == u8']') {
            out.closing_square();
            ++length;
            return length;
        }
        if (str[0] == u8',') {
            out.comma();
            str.remove_prefix(1);
            ++length;
            continue;
        }
        ULIGHT_ASSERT_UNREACHABLE(u8"Argument terminated for seemingly no reason.");
    }

    out.unexpected_eof();
    return length;
}

std::size_t match_block(Consumer& out, std::u8string_view str)
{
    if (!str.starts_with(u8'{')) {
        return 0;
    }
    out.opening_brace();
    str.remove_prefix(1);

    const std::size_t content_length = match_content_sequence(out, str, Content_Context::block);
    str.remove_prefix(content_length);

    if (str.starts_with(u8'}')) {
        out.closing_brace();
        return content_length + 2;
    }
    ULIGHT_ASSERT(str.empty());
    out.unexpected_eof();
    return content_length + 1;
}

std::size_t match_directive(Consumer& out, const std::u8string_view str)
{
    if (!str.starts_with(u8'\\')) {
        return 0;
    }
    const std::size_t name_length = match_directive_name(str.substr(1));
    if (name_length == 0) {
        return 0;
    }
    out.push_directive();
    out.directive_name(1 + name_length);

    const std::size_t args_length = match_argument_list(out, str.substr(1 + name_length));
    const std::size_t block_length = match_block(out, str.substr(1 + name_length + args_length));
    out.pop_directive();
    return 1 + name_length + args_length + block_length;
}

struct [[nodiscard]] Highlighter : Highlighter_Base {

    Highlighter(
        Non_Owning_Buffer<Token>& out,
        std::u8string_view source,
        const Highlight_Options& options
    )
        : Highlighter_Base { out, source, options }
    {
    }

    bool operator()();

private:
    void consume_content_sequence(Content_Context context)
    {
        Bracket_Levels levels {};
        while (!eof() && !is_terminated_by(context, remainder[0])) {
            consume_content(context, levels);
        }
    }

    void consume_content(Content_Context context, Bracket_Levels& levels)
    {
        if (expect_escape() || expect_directive()) {
            return;
        }
        std::size_t plain_length = 0;

        for (; plain_length < remainder.length(); ++plain_length) {
            const char8_t c = remainder[plain_length];
            if (c == u8'\\') {
                if (starts_with_escape_or_directive(remainder.substr(plain_length))) {
                    break;
                }
                continue;
            }
            if (context == Content_Context::document) {
                continue;
            }
            if (context == Content_Context::argument_value) {
                if (c == u8',') {
                    break;
                }
                if (c == u8'[') {
                    ++levels.square;
                }
                if (c == u8']' && levels.square-- == 0) {
                    break;
                }
            }
            if (c == u8'{') {
                ++levels.brace;
            }
            if (c == u8'}' && levels.brace-- == 0) {
                break;
            }
        }

        advance(plain_length);
    }

    bool expect_directive()
    {
        if (!remainder.starts_with(u8'\\')) {
            return false;
        }
        const std::size_t name_length = match_directive_name(remainder.substr(1));
        if (name_length == 0) {
            return false;
        }

        emit_and_advance(1 + name_length, Highlight_Type::markup_tag);

        expect_argument_list();
        expect_block();
        return true;
    }

    bool expect_argument_list()
    {
        if (!remainder.starts_with(u8'[')) {
            return false;
        }
        emit_and_advance(1, Highlight_Type::sym_square);

        while (!eof()) {
            consume_argument();
            if (remainder[0] == u8'}') {
                return true;
            }
            if (remainder.starts_with(u8']')) {
                emit_and_advance(1, Highlight_Type::sym_square);
                return true;
            }
            if (remainder.starts_with(u8',')) {
                emit_and_advance(1, Highlight_Type::sym_punc);
                continue;
            }
            ULIGHT_ASSERT(eof());
            break;
        }
        return true;
    }

    [[nodiscard]]
    bool expect_escape()
    {
        constexpr std::size_t sequence_length = 2;
        if (remainder.length() < sequence_length || //
            remainder[0] != u8'\\' || //
            !is_mmml_escapeable(remainder[1])) {
            return false;
        }
        emit_and_advance(sequence_length, Highlight_Type::escape);
        return true;
    }

    void consume_argument()
    {
        if (Named_Argument_Result r = match_named_argument_prefix(remainder)) {
            advance(r.leading_whitespace);
            emit_and_advance(r.name_length, Highlight_Type::id_argument);
            advance(r.trailing_whitespace);
            emit_and_advance(1, Highlight_Type::sym_punc);
        }

        consume_content_sequence(Content_Context::argument_value);
    }

    bool expect_block()
    {
        if (!remainder.starts_with(u8'{')) {
            return false;
        }
        emit_and_advance(1, Highlight_Type::sym_brace);
        consume_content_sequence(Content_Context::block);
        if (remainder.starts_with(u8'}')) {
            emit_and_advance(1, Highlight_Type::sym_brace);
        }
        else {
            ULIGHT_ASSERT(eof());
        }
        return true;
    }

    struct Dispatch_Consumer;
    struct Normal_Consumer;
    struct Comment_Consumer;
};

struct Highlighter::Normal_Consumer final : Consumer {
    Highlighter& self;

    Normal_Consumer(Highlighter& self)
        : self { self }
    {
    }

    void whitespace(std::size_t w) final
    {
        self.advance(w);
    }
    void text(std::size_t t) final
    {
        self.advance(t);
    }
    void opening_square() final
    {
        self.emit_and_advance(1, Highlight_Type::sym_square);
    }
    void closing_square() final
    {
        self.emit_and_advance(1, Highlight_Type::sym_square);
    }
    void comma() final
    {
        self.emit_and_advance(1, Highlight_Type::sym_punc);
    }
    void argument_name(std::size_t a) final
    {
        self.emit_and_advance(a, Highlight_Type::markup_attr);
    }
    void equals() final
    {
        self.emit_and_advance(1, Highlight_Type::sym_punc);
    }
    void directive_name(std::size_t d) final
    {
        self.emit_and_advance(d, Highlight_Type::markup_tag);
    }
    void opening_brace() final
    {
        self.emit_and_advance(1, Highlight_Type::sym_brace);
    }
    void closing_brace() final
    {
        self.emit_and_advance(1, Highlight_Type::sym_brace);
    }
    void escape() final
    {
        self.emit_and_advance(2, Highlight_Type::escape);
    }
    void unexpected_eof() final { }
};

struct Highlighter::Comment_Consumer final : Consumer {
    std::size_t prefix = 0;
    std::size_t content = 0;
    std::size_t suffix = 0;

private:
    std::size_t square_level = 0;
    std::size_t brace_level = 0;
    std::size_t* active_length = &prefix;

public:
    Comment_Consumer() = default;
    ~Comment_Consumer() = default;

    Comment_Consumer(const Comment_Consumer&) = delete;
    Comment_Consumer& operator=(const Comment_Consumer&) = delete;

    void reset()
    {
        prefix = 0;
        content = 0;
        suffix = 0;
        square_level = 0;
        brace_level = 0;
        active_length = &prefix;
    }

    [[nodiscard]]
    bool done() const
    {
        return active_length == &suffix;
    }

    void whitespace(std::size_t w) final
    {
        *active_length += w;
    }
    void text(std::size_t t) final
    {
        *active_length += t;
    }
    void opening_square() final
    {
        *active_length += 1;
        ++square_level;
    }
    void closing_square() final
    {
        *active_length += 1;
        --square_level;
    }
    void comma() final
    {
        *active_length += 1;
    }
    void argument_name(std::size_t a) final
    {
        *active_length += a;
    }
    void equals() final
    {
        *active_length += 1;
    }
    void directive_name(std::size_t d) final
    {
        *active_length += d;
    }
    void opening_brace() final
    {
        *active_length += 1;
        if (square_level == 0 && brace_level == 0) {
            ULIGHT_DEBUG_ASSERT(prefix != 0);
            active_length = &content;
        }
        ++brace_level;
    }
    void closing_brace() final
    {
        --brace_level;
        if (brace_level == 0 && active_length == &content) {
            active_length = &suffix;
        }
        *active_length += 1;
    }
    void escape() final
    {
        *active_length += 1;
    }
    void unexpected_eof() final
    {
        active_length = &suffix;
        ULIGHT_DEBUG_ASSERT(done());
    }
};

struct Highlighter::Dispatch_Consumer final : Consumer {
    Normal_Consumer normal;
    Comment_Consumer comment;
    Consumer* current = &normal;

    Dispatch_Consumer(Highlighter& self)
        : normal { self }
    {
    }

    void whitespace(std::size_t w) final
    {
        ULIGHT_DEBUG_ASSERT(w != 0);
        current->whitespace(w);
    }
    void text(std::size_t t) final
    {
        ULIGHT_DEBUG_ASSERT(t != 0);
        current->text(t);
    }
    void opening_square() final
    {
        current->opening_square();
    }
    void closing_square() final
    {
        current->closing_square();
    }
    void comma() final
    {
        current->comma();
    }
    void argument_name(std::size_t a) final
    {
        ULIGHT_DEBUG_ASSERT(a != 0);
        current->argument_name(a);
    }
    void equals() final
    {
        current->equals();
    }
    void directive_name(std::size_t d) final
    {
        ULIGHT_DEBUG_ASSERT(d != 0);
        const std::u8string_view name = normal.self.remainder.substr(0, d);
        if (name == u8"\\comment" || name == u8"\\-comment") {
            current = &comment;
        }
        current->directive_name(d);
    }
    void opening_brace() final
    {
        current->opening_brace();
    }
    void closing_brace() final
    {
        current->closing_brace();
    }
    void escape() final
    {
        current->escape();
    }

    void pop_directive() final
    {
        flush_special_consumers();
    }
    void unexpected_eof() final
    {
        current->unexpected_eof();
        flush_special_consumers();
    }

    void flush_special_consumers()
    {
        Highlighter& self = normal.self;
        if (comment.done()) {
            ULIGHT_ASSERT(comment.prefix != 0);
            self.emit_and_advance(comment.prefix, Highlight_Type::comment_delim);
            if (comment.content) {
                self.emit_and_advance(comment.content, Highlight_Type::comment);
            }
            if (comment.suffix) {
                ULIGHT_ASSERT(comment.suffix == 1);
                self.emit_and_advance(comment.suffix, Highlight_Type::comment_delim);
            }
            comment.reset();
        }
        current = &normal;
    }
};

bool Highlighter::operator()()
{
    Dispatch_Consumer consumer { *this };
    match_content_sequence(consumer, remainder, Content_Context::document);
    return true;
}

} // namespace
} // namespace mmml

bool highlight_mmml(
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    std::pmr::memory_resource*,
    const Highlight_Options& options
)
{
    return mmml::Highlighter { out, source, options }();
}

} // namespace ulight
