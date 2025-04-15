#ifndef ULIGHT_ULIGHT_HPP
#define ULIGHT_ULIGHT_HPP

#include <cstddef>
#include <new>
#include <span>
#include <string_view>
#include <type_traits>

#include "ulight.h"
#include "ulight/function_ref.hpp"

namespace ulight {

/// The default underlying type for scoped enumerations.
using Underlying = unsigned char;

/// See `ulight_lang`.
enum struct Lang : Underlying {
    c = ULIGHT_LANG_C,
    cpp = ULIGHT_LANG_CPP,
    css = ULIGHT_LANG_CSS,
    html = ULIGHT_LANG_HTML,
    javascript = ULIGHT_LANG_JS,
    lua = ULIGHT_LANG_LUA,
    mmml = ULIGHT_LANG_MMML,
    nasm = ULIGHT_LANG_NASM,
    none = ULIGHT_LANG_NONE,
};

[[nodiscard]]
inline Lang get_lang(std::string_view name) noexcept
{
    return Lang(ulight_get_lang(name.data(), name.length()));
}

[[nodiscard]]
inline Lang get_lang(std::u8string_view name) noexcept
{
    return get_lang({ reinterpret_cast<const char*>(name.data()), name.size() });
}

/// See `ulight_status`.
enum struct Status : Underlying {
    ok = ULIGHT_STATUS_OK,
    bad_buffer = ULIGHT_STATUS_BAD_BUFFER,
    bad_lang = ULIGHT_STATUS_BAD_LANG,
    bad_text = ULIGHT_STATUS_BAD_TEXT,
    bad_state = ULIGHT_STATUS_BAD_STATE,
    bad_code = ULIGHT_STATUS_BAD_CODE,
    bad_alloc = ULIGHT_STATUS_BAD_ALLOC,
    internal_error = ULIGHT_STATUS_INTERNAL_ERROR,
};

/// See `ulight_flag`.
enum struct Flag : Underlying {
    no_flags = ULIGHT_NO_FLAGS,
    coalesce = ULIGHT_COALESCE,
    strict = ULIGHT_STRICT,
};

[[nodiscard]]
constexpr Flag operator|(Flag x, Flag y) noexcept
{
    return Flag(Underlying(x) | Underlying(y));
}

// A table with the columns:
//   - identifier (for enumerator names) (trailing underscores may be needed to avoid C++ keywords)
//   - long string (same as identifier, but with hyphens and without trailing underscores)
//   - short string (like long string, but with each part at most four characters long)
//   - ulight_highlight_type value
#define ULIGHT_HIGHLIGHT_TYPE_ENUM_DATA(F)                                                         \
    F(error, "error", "err", ULIGHT_HL_ERROR)                                                      \
    F(comment, "comment", "cmt", ULIGHT_HL_COMMENT)                                                \
    F(comment_delim, "comment-delim", "cmt_dlim", ULIGHT_HL_COMMENT_DELIM)                         \
    F(value, "value", "val", ULIGHT_HL_VALUE)                                                      \
    F(number, "number", "num", ULIGHT_HL_NUMBER)                                                   \
    F(number_delim, "number-delim", "num_dlim", ULIGHT_HL_NUMBER_DELIM)                            \
    F(number_decor, "number-decor", "num_deco", ULIGHT_HL_NUMBER_DECOR)                            \
    F(string, "string", "str", ULIGHT_HL_STRING)                                                   \
    F(string_delim, "string-delim", "str_dlim", ULIGHT_HL_STRING_DELIM)                            \
    F(string_decor, "string-decor", "str_deco", ULIGHT_HL_STRING_DECOR)                            \
    F(escape, "escape", "esc", ULIGHT_HL_ESCAPE)                                                   \
    F(null, "null", "null", ULIGHT_HL_NULL)                                                        \
    F(bool_, "bool", "bool", ULIGHT_HL_BOOL)                                                       \
    F(this_, "this", "this", ULIGHT_HL_THIS)                                                       \
    F(macro, "macro", "mac", ULIGHT_HL_MACRO)                                                      \
    F(id, "id", "id", ULIGHT_HL_ID)                                                                \
    F(id_decl, "id-decl", "id_decl", ULIGHT_HL_ID_DECL)                                            \
    F(id_var, "id-var", "id_var", ULIGHT_HL_ID_VAR)                                                \
    F(id_var_decl, "id-var-decl", "id_var_decl", ULIGHT_HL_ID_VAR_DECL)                            \
    F(id_const, "id-const", "id_cons", ULIGHT_HL_ID_CONST)                                         \
    F(id_const_decl, "id-const-decl", "id_cons_decl", ULIGHT_HL_ID_CONST_DECL)                     \
    F(id_function, "id-function", "id_fun", ULIGHT_HL_ID_FUNCTION)                                 \
    F(id_function_decl, "id-function-decl", "id_fun_decl", ULIGHT_HL_ID_FUNCTION_DECL)             \
    F(id_type, "id-type", "id_type", ULIGHT_HL_ID_TYPE)                                            \
    F(id_type_decl, "id-type-decl", "id_type_decl", ULIGHT_HL_ID_TYPE_DECL)                        \
    F(id_module, "id-module", "id_mod", ULIGHT_HL_ID_MODULE)                                       \
    F(id_module_decl, "id-module-decl", "id_mod_decl", ULIGHT_HL_ID_MODULE_DECL)                   \
    F(id_label, "id-label", "id_labl", ULIGHT_HL_ID_LABEL)                                         \
    F(id_label_decl, "id-label-decl", "id_labl_decl", ULIGHT_HL_ID_LABEL_DECL)                     \
    F(keyword, "keyword", "kw", ULIGHT_HL_KEYWORD)                                                 \
    F(keyword_control, "keyword-control", "kw_ctrl", ULIGHT_HL_KEYWORD_CONTROL)                    \
    F(keyword_type, "keyword-type", "kw_type", ULIGHT_HL_KEYWORD_TYPE)                             \
    F(keyword_op, "keyword-op", "kw_op", ULIGHT_HL_KEYWORD_OP)                                     \
    F(attr, "attr", "attr", ULIGHT_HL_ATTR)                                                        \
    F(attr_delim, "attr-delim", "attr_dlim", ULIGHT_HL_ATTR_DELIM)                                 \
    F(diff_heading, "diff-heading", "diff_h", ULIGHT_HL_DIFF_HEADING)                              \
    F(diff_common, "diff-common", "diff_eq", ULIGHT_HL_DIFF_COMMON)                                \
    F(diff_hunk, "diff-hunk", "diff_hunk", ULIGHT_HL_DIFF_HUNK)                                    \
    F(diff_deletion, "diff-deletion", "diff_del", ULIGHT_HL_DIFF_DELETION)                         \
    F(diff_insertion, "diff-insertion", "diff_ins", ULIGHT_HL_DIFF_INSERTION)                      \
    F(markup_tag, "markup-tag", "mk_tag", ULIGHT_HL_MARKUP_TAG)                                    \
    F(markup_attr, "markup-attr", "mk_attr", ULIGHT_HL_MARKUP_ATTR)                                \
    F(markup_deletion, "markup-deletion", "mk_del", ULIGHT_HL_MARKUP_DELETION)                     \
    F(markup_insertion, "markup-insertion", "mk_ins", ULIGHT_HL_MARKUP_INSERTION)                  \
    F(markup_emph, "markup-emph", "mk_emph", ULIGHT_HL_MARKUP_EMPH)                                \
    F(markup_strong, "markup-strong", "mk_stro", ULIGHT_HL_MARKUP_STRONG)                          \
    F(markup_emph_strong, "markup-emph-strong", "mk_emph_stro", ULIGHT_HL_MARKUP_EMPH_STRONG)      \
    F(markup_underline, "markup-underline", "mk_ulin", ULIGHT_HL_MARKUP_UNDERLINE)                 \
    F(markup_emph_underline, "markup-emph-underline", "mk_emph_ulin",                              \
      ULIGHT_HL_MARKUP_EMPH_UNDERLINE)                                                             \
    F(markup_strong_underline, "markup-strong-underline", "mk_stro_ulin",                          \
      ULIGHT_HL_MARKUP_STRONG_UNDERLINE)                                                           \
    F(markup_emph_strong_underline, "markup-emph-strong-underline", "mk_emph_stro_ulin",           \
      ULIGHT_HL_MARKUP_EMPH_STRONG_UNDERLINE)                                                      \
    F(markup_strikethrough, "markup-strikethrough", "mk_strk", ULIGHT_HL_MARKUP_STRIKETHROUGH)     \
    F(markup_emph_strikethrough, "markup-emph-strikethrough", "mk_emph_strk",                      \
      ULIGHT_HL_MARKUP_EMPH_STRIKETHROUGH)                                                         \
    F(markup_strong_strikethrough, "markup-strong-strikethrough", "mk_stro_strk",                  \
      ULIGHT_HL_MARKUP_STRONG_STRIKETHROUGH)                                                       \
    F(markup_emph_strong_strikethrough, "markup-emph-strong-strikethrough", "mk_emph_stro_strk",   \
      ULIGHT_HL_MARKUP_EMPH_STRONG_STRIKETHROUGH)                                                  \
    F(markup_underline_strikethrough, "markup-underline-strikethrough", "mk_ulin_strk",            \
      ULIGHT_HL_MARKUP_UNDERLINE_STRIKETHROUGH)                                                    \
    F(markup_emph_underline_strikethrough, "markup-emph-underline-strikethrough",                  \
      "mk_emph_ulin_strk", ULIGHT_HL_MARKUP_EMPH_UNDERLINE_STRIKETHROUGH)                          \
    F(markup_strong_underline_strikethrough, "markup-strong-underline-strikethrough",              \
      "mk_stro_ulin_strk", ULIGHT_HL_MARKUP_STRONG_UNDERLINE_STRIKETHROUGH)                        \
    F(markup_emph_strong_underline_strikethrough, "markup-emph-strong-underline-strikethrough",    \
      "mk_emph_stro_ulin_strk", ULIGHT_HL_MARKUP_EMPH_STRONG_UNDERLINE_STRIKETHROUGH)              \
    F(sym, "sym", "sym", ULIGHT_HL_SYM)                                                            \
    F(sym_punc, "sym-punc", "sym_punc", ULIGHT_HL_SYM_PUNC)                                        \
    F(sym_parens, "sym-parens", "sym_par", ULIGHT_HL_SYM_PARENS)                                   \
    F(sym_square, "sym-square", "sym_sqr", ULIGHT_HL_SYM_SQUARE)                                   \
    F(sym_brace, "sym-brace", "sym_brac", ULIGHT_HL_SYM_BRACE)                                     \
    F(sym_op, "sym-op", "sym_op", ULIGHT_HL_SYM_OP)

// NOLINTNEXTLINE(bugprone-macro-parentheses)
#define ULIGHT_HIGHLIGHT_TYPE_ENUMERATOR(id, long_str, short_str, initializer) id = initializer,
#define ULIGHT_HIGHLIGHT_TYPE_LONG_STRING_CASE(id, long_str, short_str, initializer)               \
    case id: return long_str;
#define ULIGHT_HIGHLIGHT_TYPE_SHORT_STRING_CASE(id, long_str, short_str, initializer)              \
    case id: return short_str;

/// See `ulight_highlight_type`.
enum struct Highlight_Type : Underlying {
    ULIGHT_HIGHLIGHT_TYPE_ENUM_DATA(ULIGHT_HIGHLIGHT_TYPE_ENUMERATOR)
};

[[nodiscard]]
constexpr std::string_view highlight_type_long_string(Highlight_Type type) noexcept
{
    switch (type) {
        using enum Highlight_Type;
        ULIGHT_HIGHLIGHT_TYPE_ENUM_DATA(ULIGHT_HIGHLIGHT_TYPE_LONG_STRING_CASE)
    }
    return {};
}

[[nodiscard]]
constexpr std::string_view highlight_type_short_string(Highlight_Type type) noexcept
{
    switch (type) {
        using enum Highlight_Type;
        ULIGHT_HIGHLIGHT_TYPE_ENUM_DATA(ULIGHT_HIGHLIGHT_TYPE_SHORT_STRING_CASE)
    }
    return {};
}

[[deprecated("Use highlight_type_long_string or highlight_type_short_string")]] [[nodiscard]]
inline std::string_view highlight_type_id(Highlight_Type type) noexcept
{
    return highlight_type_short_string(type);
}

/// See `ulight_token`.
using Token = ulight_token;

/// See `ulight_alloc`.
[[nodiscard]]
inline void* alloc(std::size_t size, std::size_t alignment) noexcept
{
    return ulight_alloc(size, alignment);
}

/// See `ulight_free`.
inline void free(void* pointer, std::size_t size, std::size_t alignment) noexcept
{
    ulight_free(pointer, size, alignment);
}

using Alloc_Function = void*(std::size_t, std::size_t) noexcept;
using Free_Function = void(void*, std::size_t, std::size_t) noexcept;

/// See `ulight_state`.
struct [[nodiscard]] State {
    ulight_state impl;

    /// See `ulight_init`.
    State() noexcept
    {
        ulight_init(&impl);
    }

    [[nodiscard]]
    std::string_view get_source() const noexcept
    {
        return { impl.source, impl.source_length };
    }

    [[nodiscard]]
    std::u8string_view get_u8source() const noexcept
    {
        return { std::launder(reinterpret_cast<const char8_t*>(impl.source)), impl.source_length };
    }

    void set_source(std::u8string_view source) noexcept
    {
        impl.source = reinterpret_cast<const char*>(source.data());
        impl.source_length = source.size();
    }

    void set_source(std::string_view source) noexcept
    {
        impl.source = source.data();
        impl.source_length = source.size();
    }

    [[nodiscard]]
    Lang get_lang() const noexcept
    {
        return Lang(impl.lang);
    }

    void set_lang(ulight_lang lang) noexcept
    {
        impl.lang = lang;
    }

    void set_lang(Lang lang) noexcept
    {
        impl.lang = ulight_lang(lang);
    }

    [[nodiscard]]
    Flag get_flags() const noexcept
    {
        return Flag(impl.flags);
    }

    void set_flags(ulight_flag flags) noexcept
    {
        impl.flags = flags;
    }

    void set_flags(Flag flags) noexcept
    {
        impl.flags = ulight_flag(flags);
    }

    [[nodiscard]]
    std::span<Token> get_token_buffer() const noexcept
    {
        return { impl.token_buffer, impl.token_buffer_length };
    }

    void set_token_buffer(std::span<Token> buffer)
    {
        impl.token_buffer = buffer.data();
        impl.token_buffer_length = buffer.size();
    }

    void on_flush_tokens(Function_Ref<void(Token*, std::size_t)> action)
    {
        impl.flush_tokens = action.get_invoker();
        impl.flush_tokens_data = action.get_entity();
    }

    void set_html_tag_name(std::string_view name) noexcept
    {
        impl.html_tag_name = name.data();
        impl.html_tag_name_length = name.length();
    }

    void set_html_attr_name(std::string_view name) noexcept
    {
        impl.html_attr_name = name.data();
        impl.html_tag_name_length = name.length();
    }

    void set_text_buffer(std::span<char> buffer)
    {
        impl.text_buffer = buffer.data();
        impl.text_buffer_length = buffer.size();
    }

    void on_flush_text(Function_Ref<void(char*, std::size_t)> action)
    {
        impl.flush_text = action.get_invoker();
        impl.flush_text_data = action.get_entity();
    }

    /// Returns the current contents of the text buffer as a `std::span<char>`.
    [[nodiscard]]
    std::span<char> get_text_buffer() const noexcept
    {
        return { impl.text_buffer, impl.text_buffer_length };
    }

    /// Returns the current contents of the text buffer as a `std::span<char8_t>`.
    [[nodiscard]]
    std::span<char8_t> get_text_u8buffer() const noexcept
    {
        return { std::launder(reinterpret_cast<char8_t*>(impl.text_buffer)),
                 impl.text_buffer_length };
    }

    /// Returns the current contents of the text buffer as a `std::string_view`.
    [[nodiscard]]
    std::string_view get_text_buffer_string() const noexcept
    {
        return { impl.text_buffer, impl.text_buffer_length };
    }

    /// Returns the current contents of the text buffer as a `std::span<char8_t>`.
    [[nodiscard]]
    std::u8string_view get_text_buffer_u8string() const noexcept
    {
        return { std::launder(reinterpret_cast<const char8_t*>(impl.text_buffer)),
                 impl.text_buffer_length };
    }

    /// See `ulight_source_to_tokens`.
    [[nodiscard]]
    Status source_to_tokens() noexcept
    {
        return Status(ulight_source_to_tokens(&impl));
    }

    /// See `ulight_source_to_html`.
    [[nodiscard]]
    Status source_to_html() noexcept
    {
        return Status(ulight_source_to_html(&impl));
    }

    [[nodiscard]]
    std::string_view get_error_string() const noexcept
    {
        return { impl.error, impl.error_length };
    }

    [[nodiscard]]
    std::u8string_view get_error_u8string() const noexcept
    {
        return { reinterpret_cast<const char8_t*>(impl.error), impl.error_length };
    }
};

static_assert(std::is_trivially_copyable_v<State>);

} // namespace ulight

#endif
