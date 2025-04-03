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
    xml = ULIGHT_LANG_XML,
    cpp = ULIGHT_LANG_CPP,
    html = ULIGHT_LANG_HTML,
    lua = ULIGHT_LANG_LUA,
    mmml = ULIGHT_LANG_MMML,
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

/// See `ulight_highlight_type`.
enum struct Highlight_Type : Underlying {
    error = ULIGHT_HL_ERROR,
    comment = ULIGHT_HL_COMMENT,
    comment_delimiter = ULIGHT_HL_COMMENT_DELIMITER,
    value = ULIGHT_HL_VALUE,
    number = ULIGHT_HL_NUMBER,
    string = ULIGHT_HL_STRING,
    escape = ULIGHT_HL_ESCAPE,
    null = ULIGHT_HL_NULL,
    bool_ = ULIGHT_HL_BOOL,
    this_ = ULIGHT_HL_THIS,
    macro = ULIGHT_HL_MACRO,
    id = ULIGHT_HL_ID,
    id_decl = ULIGHT_HL_ID_DECL,
    id_use = ULIGHT_HL_ID_USE,
    id_var_decl = ULIGHT_HL_ID_VAR_DECL,
    id_var_use = ULIGHT_HL_ID_VAR_USE,
    id_const_decl = ULIGHT_HL_ID_CONST_DECL,
    id_const_use = ULIGHT_HL_ID_CONST_USE,
    id_type_decl = ULIGHT_HL_ID_TYPE_DECL,
    id_type_use = ULIGHT_HL_ID_TYPE_USE,
    id_module_decl = ULIGHT_HL_ID_MODULE_DECL,
    id_module_use = ULIGHT_HL_ID_MODULE_USE,
    keyword = ULIGHT_HL_KEYWORD,
    keyword_control = ULIGHT_HL_KEYWORD_CONTROL,
    keyword_type = ULIGHT_HL_KEYWORD_TYPE,
    attr = ULIGHT_HL_ATTR,
    attr_delim = ULIGHT_HL_ATTR_DELIM,
    diff_heading = ULIGHT_HL_DIFF_HEADING,
    diff_common = ULIGHT_HL_DIFF_COMMON,
    diff_hunk = ULIGHT_HL_DIFF_HUNK,
    diff_deletion = ULIGHT_HL_DIFF_DELETION,
    diff_insertion = ULIGHT_HL_DIFF_INSERTION,
    markup_tag = ULIGHT_HL_MARKUP_TAG,
    markup_attr = ULIGHT_HL_MARKUP_ATTR,
    sym = ULIGHT_HL_SYM,
    sym_punc = ULIGHT_HL_SYM_PUNC,
    sym_parens = ULIGHT_HL_SYM_PARENS,
    sym_square = ULIGHT_HL_SYM_SQUARE,
    sym_brace = ULIGHT_HL_SYM_BRACE,
    sym_op = ULIGHT_HL_SYM_OP
};

[[nodiscard]]
constexpr std::string_view ulight_highlight_type_id(Highlight_Type type) noexcept
{
    // Design note:
    // All names should be underscore-separated blocks of up to four characters.
    // The abbreviations should always be distinct if they abbreviate different words.
    // For example, "del" stands for "deletion", and "dlm" stands for "delimiter".
    switch (type) {
        using enum Highlight_Type;
    case error: return "err";
    case comment: return "cmt";
    case comment_delimiter: return "cmt_dlm";
    case value: return "val";
    case number: return "num";
    case string: return "str";
    case escape: return "esc";
    case null: return "null";
    case bool_: return "bool";
    case this_: return "this";
    case macro: return "mac";
    case id: return "id";
    case id_decl: return "id_dcl";
    case id_use: return "id_use";
    case id_var_decl: return "id_var_dcl";
    case id_var_use: return "id_var_use";
    case id_const_decl: return "id_cons_dcl";
    case id_const_use: return "id_cons_use";
    case id_type_decl: return "id_type_dcl";
    case id_type_use: return "id_type_use";
    case id_module_decl: return "id_mod_dcl";
    case id_module_use: return "id_mod_use";
    case keyword: return "kw";
    case keyword_control: return "kw_ctrl";
    case keyword_type: return "kw_type";
    case attr: return "attr";
    case attr_delim: return "attr_dlim";
    case diff_heading: return "df_h";
    case diff_common: return "df_eq";
    case diff_hunk: return "df_hunk";
    case diff_deletion: return "df_del";
    case diff_insertion: return "df_ins";
    case markup_tag: return "mk_tag";
    case markup_attr: return "mk_attr";
    case sym: return "sym";
    case sym_punc: return "sym_punc";
    case sym_parens: return "sym_par";
    case sym_square: return "sym_sqr";
    case sym_brace: return "sym_brac";
    case sym_op: return "sym_op";
    default: return "";
    }
}

[[nodiscard]]
inline std::string_view highlight_type_id(Highlight_Type type) noexcept
{
    const ulight_string_view result = ulight_highlight_type_id(ulight_highlight_type(type));
    return { result.text, result.length };
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

    void set_lang(Lang lang) noexcept
    {
        impl.lang = ulight_lang(lang);
    }

    [[nodiscard]]
    Flag get_flags() const noexcept
    {
        return Flag(impl.flags);
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
