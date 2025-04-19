#include <cstddef>
#include <expected>
#include <memory_resource>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

#include "ulight/ulight.hpp"

#include "ulight/impl/assert.hpp"
#include "ulight/impl/buffer.hpp"
#include "ulight/impl/highlight.hpp"
#include "ulight/impl/unicode.hpp"

#include "ulight/impl/lang/mmml.hpp"
#include "ulight/impl/lang/mmml_chars.hpp"

namespace ulight {

namespace {

enum struct Content_Context : Underlying {
    document,
    argument_value,
    block
};

[[nodiscard]]
constexpr bool is_terminated_by(Content_Context context, char8_t c)
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

struct [[nodiscard]] Parser {
private:
    struct [[nodiscard]] Scoped_Attempt {
    private:
        Parser* m_self;
        const std::size_t m_initial_pos;
        const std::size_t m_initial_size;

    public:
        Scoped_Attempt(Parser& self)
            : m_self { &self }
            , m_initial_pos { self.m_pos }
            , m_initial_size { self.m_out.size() }
        {
        }

        Scoped_Attempt(const Scoped_Attempt&) = delete;
        Scoped_Attempt& operator=(const Scoped_Attempt&) = delete;

        void commit()
        {
            ULIGHT_ASSERT(m_self);
            m_self = nullptr;
        }

        void abort()
        {
            ULIGHT_ASSERT(m_self);
            ULIGHT_ASSERT(m_self->m_out.size() >= m_initial_size);

            m_self->m_pos = m_initial_pos;
            m_self->m_out.resize(m_initial_size);

            m_self = nullptr;
        }

        ~Scoped_Attempt() // NOLINT(bugprone-exception-escape)
        {
            if (m_self) {
                abort();
            }
        }
    };

    std::pmr::vector<AST_Instruction>& m_out;
    std::u8string_view m_source;
    std::size_t m_pos = 0;

public:
    Parser(std::pmr::vector<AST_Instruction>& out, std::u8string_view source)
        : m_out { out }
        , m_source { source }
    {
    }

    void operator()()
    {
        const std::size_t document_instruction_index = m_out.size();
        m_out.push_back({ AST_Instruction_Type::push_document, 0 });
        const std::size_t content_amount = match_content_sequence(Content_Context::document);
        m_out[document_instruction_index].n = content_amount;
        m_out.push_back({ AST_Instruction_Type::pop_document, 0 });
    }

private:
    Scoped_Attempt attempt()
    {
        return Scoped_Attempt { *this };
    }

    /// @brief Returns all remaining text as a `std::string_view_type`, from the current parsing
    /// position to the end of the file.
    /// @return All remaining text.
    [[nodiscard]]
    std::u8string_view peek_all() const
    {
        return m_source.substr(m_pos);
    }

    /// @brief Returns the next character and advances the parser position.
    /// @return The popped character.
    /// @throws Throws if `eof()`.
    char8_t pop()
    {
        const char8_t c = peek();
        ++m_pos;
        return c;
    }

    char32_t pop_code_point()
    {
        const auto [code_point, length] = peek_code_point();
        m_pos += std::size_t(length);
        return code_point;
    }

    /// @brief Returns the next character.
    /// @return The next character.
    /// @throws Throws if `eof()`.
    [[nodiscard]]
    char8_t peek() const
    {
        ULIGHT_ASSERT(!eof());
        return m_source[m_pos];
    }

    [[nodiscard]]
    utf8::Code_Point_And_Length peek_code_point() const
    {
        ULIGHT_ASSERT(!eof());
        const std::u8string_view remainder { m_source.substr(m_pos) };
        const std::expected<utf8::Code_Point_And_Length, utf8::Error_Code> result
            = utf8::decode_and_length(remainder);
        ULIGHT_ASSERT(result);
        return *result;
    }

    /// @return `true` if the parser is at the end of the file, `false` otherwise.
    [[nodiscard]]
    bool eof() const
    {
        return m_pos == m_source.length();
    }

    /// @return `peek_all().starts_with(text)`.
    [[nodiscard]]
    bool peek(std::u8string_view text) const
    {
        return peek_all().starts_with(text);
    }

    /// @brief Checks whether the next character matches an expected value without advancing
    /// the parser.
    /// @param c the character to test
    /// @return `true` if the next character equals `c`, `false` otherwise.
    [[nodiscard]]
    bool peek(char8_t c) const
    {
        return !eof() && m_source[m_pos] == c;
    }

    /// @brief Checks whether the parser is at the start of a directive.
    /// Namely, has to be `\\` and not be the start of an escape sequence such as `\\\\` for
    /// this to be the case. This function can have false positives in the sense that if the
    /// subsequent directive is ill-formed, the guess was optimistic, and there isn't actually a
    /// directive there. However, it has no false negatives.
    /// @return `true` if the parser is at the start of a directive, `false` otherwise.
    [[nodiscard]]
    bool peek_possible_directive() const
    {
        const std::u8string_view rest = peek_all();
        return !rest.empty() //
            && rest[0] == '\\' //
            && (rest.length() <= 1 || !is_mmml_escapeable(char8_t(rest[1])));
    }

    /// @brief Checks whether the next character satisfies a predicate without advancing
    /// the parser.
    /// @param predicate the predicate to test
    /// @return `true` if the next character satisfies `predicate`, `false` otherwise.
    bool peek(bool predicate(char8_t)) const
    {
        return !eof() && predicate(m_source[m_pos]);
    }

    [[nodiscard]]
    bool expect(char8_t c)
    {
        if (!peek(c)) {
            return false;
        }
        ++m_pos;
        return true;
    }

    [[nodiscard]]
    bool expect(bool predicate(char8_t))
    {
        if (eof()) {
            return false;
        }
        const char8_t c = m_source[m_pos];
        if (!predicate(c)) {
            return false;
        }
        // This function is only safe to call when we have expectations towards ASCII characters.
        // Any non-ASCII character should have already been rejected.
        ULIGHT_ASSERT(is_ascii(c));
        ++m_pos;
        return true;
    }

    [[nodiscard]]
    bool expect(bool predicate(char32_t))
    {
        if (eof()) {
            return false;
        }
        const auto [code_point, length] = peek_code_point();
        if (!predicate(code_point)) {
            return false;
        }
        m_pos += std::size_t(length);
        return true;
    }

    [[nodiscard]]
    bool expect_literal(std::u8string_view text)
    {
        if (!peek(text)) {
            return false;
        }
        m_pos += text.length();
        return true;
    }

    /// @brief Matches a (possibly empty) sequence of characters matching the predicate.
    /// @return The amount of characters matched.
    [[nodiscard]]
    std::size_t match_char_sequence(bool predicate(char8_t))
    {
        const std::size_t initial = m_pos;
        while (expect(predicate)) { }
        return m_pos - initial;
    }

    [[nodiscard]]
    std::size_t match_char_sequence(bool predicate(char32_t))
    {
        const std::size_t initial = m_pos;
        while (expect(predicate)) { }
        return m_pos - initial;
    }

    [[nodiscard]]
    std::size_t match_directive_name()
    {
        constexpr bool (*predicate)(char32_t) = is_mmml_directive_name_character;
        return peek(is_ascii_digit) ? 0 : match_char_sequence(predicate);
    }

    [[nodiscard]]
    std::size_t match_argument_name()
    {
        constexpr bool (*predicate)(char32_t) = is_mmml_argument_name_character;
        return peek(is_ascii_digit) ? 0 : match_char_sequence(predicate);
    }

    std::size_t match_whitespace()
    {
        constexpr bool (*predicate)(char8_t) = is_html_whitespace;
        return match_char_sequence(predicate);
    }

    [[nodiscard]]
    std::size_t match_content_sequence(Content_Context context)
    {
        Bracket_Levels levels {};
        std::size_t elements = 0;

        for (; !eof(); ++elements) {
            if (is_terminated_by(context, peek())) {
                break;
            }
            // TODO: perhaps we could simplify this by making try_match_content
            //       the loop condition.
            //       After all, that function also checks for termination and EOF.
            const bool success = try_match_content(context, levels);
            ULIGHT_ASSERT(success);
        }

        return elements;
    }

    struct Bracket_Levels {
        std::size_t square = 0;
        std::size_t brace = 0;
    };

    /// @brief Attempts to match the next piece of content,
    /// which is an escape sequence, directive, or plaintext.
    ///
    /// Returns `false` if none of these could be matched.
    /// This may happen because the parser is located at e.g. a `}` and the given `context`
    /// is terminated by `}`.
    /// It may also happen if the parser has already reached the EOF.
    [[nodiscard]]
    bool try_match_content(Content_Context context, Bracket_Levels& levels)
    {
        if (peek(u8'\\') && (try_match_escaped() || try_match_directive())) {
            return true;
        }

        const std::size_t initial_pos = m_pos;

        for (; !eof(); ++m_pos) {
            const char8_t c = m_source[m_pos];
            if (c == u8'\\') {
                const std::u8string_view remainder { m_source.substr(m_pos + 1) };

                // Trailing \ at the end of the file.
                // No need to break, we'll just run into it next iteration.
                if (remainder.empty()) {
                    continue;
                }
                // Escape sequence such as `\{`.
                // We treat these as separate in the AST, not as content.
                if (is_mmml_escapeable(remainder.front())) {
                    break;
                }
                // Directive names; also not part of content.
                // No matter what, a backslash followed by a directive name character forms a
                // directive because the remaining arguments and the block are optional.
                // I.e. we can break with certainty despite only having examined one character.
                const auto [next_point, _] = utf8::decode_and_length_or_throw(remainder);
                if (!is_ascii_digit(next_point) && is_mmml_directive_name_character(next_point)) {
                    break;
                }
                continue;
            }
            // At the document level, we don't care about brace mismatches,
            // commas, etc.
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

        ULIGHT_ASSERT(m_pos >= initial_pos);
        if (m_pos == initial_pos) {
            return false;
        }

        m_out.push_back({ AST_Instruction_Type::text, m_pos - initial_pos });
        return true;
    }

    [[nodiscard]]
    bool try_match_directive()
    {
        Scoped_Attempt a = attempt();

        if (!expect('\\')) {
            return {};
        }
        const std::size_t name_length = match_directive_name();
        if (name_length == 0) {
            return false;
        }

        m_out.push_back({ AST_Instruction_Type::push_directive, name_length + 1 });

        try_match_argument_list();
        try_match_block();

        m_out.push_back({ AST_Instruction_Type::pop_directive, 0 });

        a.commit();
        return true;
    }

    // intentionally discardable
    bool try_match_argument_list()
    {
        Scoped_Attempt a = attempt();

        if (!expect(u8'[')) {
            return {};
        }
        const std::size_t arguments_instruction_index = m_out.size();
        m_out.push_back({ AST_Instruction_Type::push_arguments, 0 });

        for (std::size_t i = 0; try_match_argument(); ++i) {
            if (expect(u8']')) {
                m_out[arguments_instruction_index].n = i + 1;
                m_out.push_back({ AST_Instruction_Type::pop_arguments });
                a.commit();
                return true;
            }
            if (expect(u8',')) {
                m_out.push_back({ AST_Instruction_Type::argument_comma });
                continue;
            }
            ULIGHT_ASSERT_UNREACHABLE(
                u8"Successfully matched arguments must be followed by ']' or ','"
            );
        }

        return false;
    }

    [[nodiscard]]
    bool try_match_escaped()
    {
        constexpr std::size_t sequence_length = 2;

        if (m_pos + sequence_length <= m_source.size() //
            && m_source[m_pos] == u8'\\' //
            && is_mmml_escapeable(char8_t(m_source[m_pos + 1]))) //
        {
            m_pos += sequence_length;
            m_out.push_back({ AST_Instruction_Type::escape, sequence_length });
            return true;
        }
        return false;
    }

    [[nodiscard]]
    bool try_match_argument()
    {
        if (eof()) {
            return false;
        }
        Scoped_Attempt a = attempt();

        const std::size_t argument_instruction_index = m_out.size();
        m_out.push_back({ AST_Instruction_Type::push_argument });

        try_match_argument_name();

        const std::optional<std::size_t> result = try_match_trimmed_argument_value();
        if (!result) {
            return false;
        }

        m_out[argument_instruction_index].n = *result;
        m_out.push_back({ AST_Instruction_Type::pop_argument });

        a.commit();
        return true;
    }

    /// @brief Matches the name of an argument, including any surrounding whitespace and the `=`
    /// character following it.
    /// If the argument couldn't be matched, returns `false` and keeps the parser state unchanged.
    bool try_match_argument_name()
    {
        Scoped_Attempt a = attempt();

        const std::size_t leading_whitespace = match_whitespace();
        if (leading_whitespace != 0) {
            m_out.push_back({ AST_Instruction_Type::skip, leading_whitespace });
        }

        if (eof()) {
            return false;
        }

        const std::size_t name_length = match_argument_name();
        m_out.push_back({ AST_Instruction_Type::argument_name, name_length });

        if (name_length == 0) {
            return false;
        }

        const std::size_t trailing_whitespace = match_whitespace();
        if (eof()) {
            return false;
        }

        if (!expect(u8'=')) {
            return false;
        }

        m_out.push_back({ AST_Instruction_Type::skip, trailing_whitespace });
        m_out.push_back({ AST_Instruction_Type::argument_equal });
        a.commit();
        return true;
    }

    [[nodiscard]]
    std::optional<std::size_t> try_match_trimmed_argument_value()
    {
        Scoped_Attempt a = attempt();

        const std::size_t leading_whitespace = match_whitespace();
        if (leading_whitespace != 0) {
            m_out.push_back({ AST_Instruction_Type::skip, leading_whitespace });
        }

        const std::size_t content_amount = match_content_sequence(Content_Context::argument_value);
        if (eof() || peek(u8'}')) {
            return {};
        }
        // match_content_sequence is very aggressive, so I think at this point,
        // we have to be at the end of an argument due to a comma separator or closing square.
        const char8_t c = m_source[m_pos];
        ULIGHT_ASSERT(c == u8',' || c == u8']');

        trim_trailing_whitespace_in_matched_content();

        a.commit();
        return content_amount;
    }

    /// @brief Trims trailing whitespace in just matched content.
    ///
    /// This is done by splitting the most recently written instruction
    /// into `text` and `skip` if that instruction is `text`.
    /// If the most recent instruction is entirely made of whitespace,
    /// it is simply replaced with `skip`.
    void trim_trailing_whitespace_in_matched_content()
    {
        ULIGHT_ASSERT(!m_out.empty());

        AST_Instruction& latest = m_out.back();
        if (latest.type != AST_Instruction_Type::text) {
            return;
        }
        const std::size_t total_length = latest.n;
        ULIGHT_ASSERT(total_length != 0);

        const std::size_t text_begin = m_pos - total_length;

        const std::u8string_view last_text = m_source.substr(text_begin, total_length);
        const std::size_t last_non_white = last_text.find_last_not_of(u8" \t\r\n\f");
        const std::size_t non_white_length = last_non_white + 1;

        if (last_non_white == std::u8string_view::npos) {
            latest.type = AST_Instruction_Type::skip;
        }
        else if (non_white_length < total_length) {
            latest.n = non_white_length;
            m_out.push_back({ AST_Instruction_Type::skip, total_length - non_white_length });
        }
        else {
            ULIGHT_ASSERT(non_white_length == total_length);
        }
    }

    bool try_match_block()
    {
        if (!expect(u8'{')) {
            return {};
        }

        Scoped_Attempt a = attempt();

        const std::size_t block_instruction_index = m_out.size();
        m_out.push_back({ AST_Instruction_Type::push_block });

        // A possible optimization should be to find the closing brace and then run the parser
        // on the brace-enclosed block.
        // This would prevent ever discarding any matched content, but might not be worth it.
        //
        // I suspect we only have to discard if we reach the EOF unexpectedly,
        // and that seems like a broken file anyway.
        const std::size_t elements = match_content_sequence(Content_Context::block);

        if (!expect(u8'}')) {
            return {};
        }

        m_out[block_instruction_index].n = elements;
        m_out.push_back({ AST_Instruction_Type::pop_block });

        a.commit();
        return elements;
    }
};

} // namespace

void parse(std::pmr::vector<AST_Instruction>& out, std::u8string_view source)
{
    Parser { out, source }();
}

bool highlight_mmml(
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    std::pmr::memory_resource* memory,
    const Highlight_Options& options
)
{
    std::pmr::vector<AST_Instruction> instructions { memory };
    parse(instructions, source);
    highlight_mmml(out, source, instructions, options);
    return true;
}

void highlight_mmml( //
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    std::span<const AST_Instruction> instructions,
    const Highlight_Options& options
)
{
    std::size_t index = 0;
    const auto emit = [&](std::size_t length, Highlight_Type type) {
        ULIGHT_DEBUG_ASSERT(length != 0);
        const bool coalesce = options.coalescing //
            && !out.empty() //
            && Highlight_Type(out.back().type) == type //
            && out.back().begin + out.back().length == index;
        if (coalesce) {
            out.back().length += length;
        }
        else {
            out.emplace_back(index, length, Underlying(type));
        }
        index += length;
    };

    // Indicates how deep we are in a comment.
    //   0: Not in a comment.
    //   1: Within the directive name, arguments, etc. but not in the comment block.
    // > 1: In the comment block.
    std::size_t in_comment = 0;
    std::size_t comment_delimiter_length = 0;
    std::size_t comment_content_length = 0;

    for (const auto& i : instructions) {
        using enum AST_Instruction_Type;
        if (in_comment != 0) {
            std::size_t& target
                = in_comment > 1 ? comment_content_length : comment_delimiter_length;
            switch (i.type) {
            case skip:
            case escape:
            case text:
            case argument_name:
            case push_directive: {
                target += i.n;
                break;
            }
            case pop_directive: {
                if (in_comment == 1) {
                    in_comment = 0;
                }
                break;
            }
            case argument_equal:
            case argument_comma:
            case push_arguments:
            case pop_arguments: {
                ++target;
                break;
            }

            case push_document:
            case pop_document:
            case push_argument:
            case pop_argument: break;

            case push_block: {
                ++target;
                if (in_comment++ <= 1) {
                    emit(comment_delimiter_length, Highlight_Type::comment_delim);
                }
                break;
            }
            case pop_block: {
                if (--in_comment == 1) {
                    if (comment_content_length != 0) {
                        emit(comment_content_length, Highlight_Type::comment);
                    }
                    emit(1, Highlight_Type::comment_delim);
                }
                else {
                    ++comment_content_length;
                }
                break;
            }
            }
        }
        else {
            switch (i.type) {
            case skip: //
                index += i.n;
                break;
            case escape: //
                emit(i.n, Highlight_Type::escape);
                break;
            case text: //
                index += i.n;
                break;
            case argument_name: //
                emit(i.n, Highlight_Type::markup_attr);
                break;
            case push_directive: {
                const std::u8string_view directive_name = source.substr(index, i.n);
                // TODO: highlight comment contents specially,
                //       perhaps by recursing into another function that handles comments
                if (directive_name == u8"\\comment" || directive_name == u8"\\-comment") {
                    in_comment = 1;
                    comment_delimiter_length = i.n;
                    comment_content_length = 0;
                }
                else {
                    emit(i.n, Highlight_Type::markup_tag);
                }
                break;
            }

            case argument_equal: // =
            case argument_comma: // ,
                emit(1, Highlight_Type::sym);
                break;
            case push_arguments: // [
            case pop_arguments: // ]
                emit(1, Highlight_Type::sym_square);
                break;
            case push_block: // {
            case pop_block: // }
                emit(1, Highlight_Type::sym_brace);
                break;

            case push_document:
            case pop_document:
            case push_argument:
            case pop_argument:
            case pop_directive: break;
            }
        }
    }
}

} // namespace ulight
