#include <algorithm>
#include <string_view>

#include "ulight/ulight.hpp"

#include "ulight/impl/highlight.hpp"
#include "ulight/impl/highlighter.hpp"

#include "ulight/impl/lang/bash.hpp"
#include "ulight/impl/lang/bash_chars.hpp"

namespace ulight {
namespace bash {

namespace {

constexpr std::u8string_view token_type_codes[] {
    ULIGHT_BASH_TOKEN_ENUM_DATA(ULIGHT_BASH_TOKEN_CODE8)
};

static_assert(std::ranges::is_sorted(token_type_codes));

// clang-format off
constexpr unsigned char token_type_lengths[] {
    ULIGHT_BASH_TOKEN_ENUM_DATA(ULIGHT_BASH_TOKEN_LENGTH)
};
// clang-format on

constexpr Highlight_Type token_type_highlights[] {
    ULIGHT_BASH_TOKEN_ENUM_DATA(ULIGHT_BASH_TOKEN_HIGHLIGHT_TYPE)
};

[[nodiscard]] [[maybe_unused]]
std::u8string_view token_type_code(Token_Type type)
{
    return token_type_codes[std::size_t(type)];
}

[[nodiscard]] [[maybe_unused]]
std::size_t token_type_length(Token_Type type)
{
    return token_type_lengths[std::size_t(type)];
}

[[nodiscard]] [[maybe_unused]]
Highlight_Type token_type_highlight(Token_Type type)
{
    return token_type_highlights[std::size_t(type)];
}

} // namespace

String_Result match_single_quoted_string(std::u8string_view str)
{
    // https://www.gnu.org/software/bash/manual/bash.html#Single-Quotes
    if (!str.starts_with(u8'\'')) {
        return {};
    }
    const std::size_t closing_index = str.find(u8'\'', 1);
    if (closing_index == std::u8string_view::npos) {
        return { .length = str.length(), .terminated = false };
    }
    return { .length = closing_index + 1, .terminated = true };
}

std::size_t match_comment(std::u8string_view str)
{
    // https://www.gnu.org/software/bash/manual/bash.html#Definitions-1
    if (!str.starts_with(u8'#')) {
        return 0;
    }
    const std::size_t line_length = str.find(u8'\n', 1);
    return line_length == std::u8string_view::npos ? str.length() : line_length;
}

std::size_t match_blank(std::u8string_view str)
{
    const auto predicate = [](char8_t c) { return is_bash_blank(c); };
    const auto* const data_end = str.data() + str.size();
    const auto* const end = std::ranges::find_if_not(str.data(), data_end, predicate);
    return std::size_t(end - str.data());
}

bool starts_with_substitution(std::u8string_view str)
{
    return str.length() >= 2 //
        && str[0] == u8'$' //
        && is_bash_parameter_substitution_start(str[1]);
}

std::size_t match_identifier(std::u8string_view str)
{
    if (str.empty()) {
        return 0;
    }
    if (!is_bash_identifier_start(str[0])) {
        return 0;
    }
    std::size_t length = 1;
    for (; length < str.length(); ++length) {
        if (!is_bash_identifier(str[length])) {
            return length;
        }
    }
    return length;
}

std::optional<Token_Type> match_operator(std::u8string_view str)
{
    using enum Token_Type;
    if (str.empty()) {
        return {};
    }
    switch (str[0]) {
    case u8'!': return exclamation;
    case u8'&':
        return str.starts_with(u8"&&") ? amp_amp
            : str.starts_with(u8"&>>") ? amp_greater_greater
            : str.starts_with(u8"&>")  ? amp_greater
                                       : amp;
    case u8'(': return left_parens;
    case u8')': return right_parens;
    case u8'*': return plus;
    case u8'-': return minus;
    case u8':': return colon;
    case u8';': return semicolon;
    case u8'<':
        return str.starts_with(u8"<<<") ? less_less_less
            : str.starts_with(u8"<<")   ? less_less
            : str.starts_with(u8"<&")   ? less_amp
            : str.starts_with(u8"<>")   ? less_greater
                                        : less;
    case u8'=': return equal;
    case u8'>':
        return str.starts_with(u8">>") ? greater_greater
            : str.starts_with(u8">&")  ? greater_amp
                                       : greater;
    case u8'?': return question;
    case u8'@': return at;
    case u8'[': return str.starts_with(u8"[[") ? left_square_square : left_square;
    case u8']': return str.starts_with(u8"]]") ? right_square_square : right_square;
    case u8'{': return left_brace;
    case u8'|': return str.starts_with(u8"||") ? pipe_pipe : pipe;
    case u8'}': return right_brace;
    case u8'~': return tilde;
    default: break;
    }
    return {};
}

namespace {

struct Highlighter : Highlighter_Base {
private:
    enum struct Context : Underlying {
        file,
        parameter_sub,
        command_sub,
    };

    enum struct State : Underlying {
        before_command,
        in_command,
        before_argument,
        in_argument,
        parameter_sub,
    };

    State state = State::before_command;

public:
    Highlighter(
        Non_Owning_Buffer<Token>& out,
        std::u8string_view source,
        const Highlight_Options& options
    )
        : Highlighter_Base { out, source, options }
    {
    }

    bool operator()()
    {
        consume_commands(Context::file);
        return true;
    }

private:
    void consume_commands(Context context)
    {
        while (!remainder.empty()) {
            switch (remainder[0]) {
            case u8'\\': {
                consume_escape_character();
                continue;
            }
            case u8'\'': {
                const String_Result string = match_single_quoted_string(remainder);
                highlight_string(string);
                continue;
            }
            case u8'"': {
                emit_and_advance(1, Highlight_Type::string_delim);
                consume_double_quoted_string();
                continue;
            }
            case u8'#': {
                const std::size_t length = match_comment(remainder);
                emit_and_advance(1, Highlight_Type::comment_delim);
                if (length > 1) {
                    emit_and_advance(length - 1, Highlight_Type::comment);
                }
                continue;
            }
            case u8' ':
            case u8'\t': {
                const std::size_t length = match_blank(remainder);
                advance(length);
                if (state == State::in_command || state == State::in_argument) {
                    state = State::before_argument;
                }
                continue;
            }
            case u8'\v':
            case u8'\r':
            case u8'\n': {
                advance(1);
                state = State::before_command;
                continue;
            }
            case u8'$': {
                if (starts_with_substitution(remainder)) {
                    consume_substitution();
                }
                else {
                    consume_word(context);
                }
                continue;
            }
            case u8'|':
            case u8'&':
            case u8';':
            case u8'(':
            case u8'<':
            case u8'>': {
                const std::optional<Token_Type> op = match_operator(remainder);
                ULIGHT_ASSERT(op);
                emit_and_advance(token_type_length(*op), Highlight_Type::sym_op);
                continue;
            }
            case u8')': {
                if (context == Context::command_sub) {
                    emit_and_advance(1, Highlight_Type::escape);
                    return;
                }
                emit_and_advance(1, Highlight_Type::sym_parens);
                continue;
            }
            case u8'}': {
                if (context == Context::parameter_sub) {
                    emit_and_advance(1, Highlight_Type::escape);
                    return;
                }
                emit_and_advance(1, Highlight_Type::sym_brace);
                continue;
            }
            default: {
                consume_word(context);
                continue;
            }
            }
        }
    }

    void consume_word(Context context)
    {
        std::size_t length = 0;
        for (; length < remainder.length(); ++length) {
            if (is_bash_unquoted_terminator(remainder[length])
                || starts_with_substitution(remainder.substr(length))
                || (context == Context::parameter_sub && remainder[length] == u8'}')) {
                break;
            }
        }
        ULIGHT_ASSERT(length != 0);
        switch (state) {
        case State::before_command:
        case State::in_command: {
            emit_and_advance(length, Highlight_Type::shell_command);
            state = State::in_command;
            break;
        }
        case State::before_argument: {
            const auto highlight = remainder.starts_with(u8'-') ? Highlight_Type::shell_option
                                                                : Highlight_Type::string;
            emit_and_advance(length, highlight);
            state = State::in_argument;
            break;
        }
        case State::in_argument: {
            emit_and_advance(length, Highlight_Type::string);
            break;
        }
        case State::parameter_sub: {
            emit_and_advance(length, Highlight_Type::id_var);
            break;
        }
        }
    }

    void consume_escape_character()
    {
        if (remainder.starts_with(u8"\\\n")) {
            emit_and_advance(1, Highlight_Type::escape);
            advance(1);
        }
        else {
            emit_and_advance(std::min(2uz, remainder.length()), Highlight_Type::escape);
        }
    }

    void highlight_string(String_Result string)
    {
        ULIGHT_ASSERT(string);
        emit_and_advance(1, Highlight_Type::string_delim);
        const std::size_t content_length = string.length - (string.terminated ? 2 : 1);
        if (content_length != 0) {
            emit_and_advance(content_length, Highlight_Type::string);
        }
        if (string.terminated) {
            emit_and_advance(1, Highlight_Type::string_delim);
        }
    }

    void consume_double_quoted_string()
    {
        std::size_t chars = 0;
        const auto flush_chars = [&] {
            if (chars != 0) {
                emit_and_advance(chars, Highlight_Type::string);
                chars = 0;
            }
        };

        for (; chars < remainder.length();) {
            if (remainder[chars] == u8'\"') {
                flush_chars();
                emit_and_advance(1, Highlight_Type::string_delim);
                return;
            }
            if (remainder[chars] == u8'\\' //
                && chars + 1 < remainder.length() //
                && is_bash_escapable_in_double_quotes(remainder[chars + 1])) {
                flush_chars();
                emit_and_advance(2, Highlight_Type::escape);
                continue;
            }
            if (starts_with_substitution(remainder.substr(chars))) {
                flush_chars();
                consume_substitution();
                continue;
            }
            ++chars;
        }
        flush_chars();
    }

    void consume_substitution()
    {
        ULIGHT_ASSERT(remainder.size() >= 2 && remainder.starts_with(u8'$'));
        const char8_t next = remainder[1];
        if (next == u8'{') {
            emit_and_advance(2, Highlight_Type::escape);
            state = State::parameter_sub;
            consume_commands(Context::parameter_sub);
            return;
        }
        if (next == u8'(') {
            emit_and_advance(2, Highlight_Type::escape);
            state = State::before_command;
            consume_commands(Context::command_sub);
            return;
        }

        const auto update_state = [&] {
            if (state == State::before_command) {
                state = State::in_command;
            }
            else if (state == State::before_argument) {
                state = State::in_argument;
            };
        };

        if (is_bash_special_parameter(next)) {
            emit_and_advance(2, Highlight_Type::escape);
            update_state();
            return;
        }
        if (const std::size_t id = match_identifier(remainder.substr(1))) {
            emit_and_advance(id + 1, Highlight_Type::escape);
            update_state();
            return;
        }
        ULIGHT_ASSERT_UNREACHABLE(u8"No substitution to consume.");
    }
};

} // namespace
} // namespace bash

bool highlight_bash(
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    std::pmr::memory_resource*,
    const Highlight_Options& options
)
{
    return bash::Highlighter { out, source, options }();
}

} // namespace ulight
