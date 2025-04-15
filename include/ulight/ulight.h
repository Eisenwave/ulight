#ifndef ULIGHT_ULIGHT_H
#define ULIGHT_ULIGHT_H
// NOLINTBEGIN

#include <stdalign.h>
#include <stddef.h>
#include <stdlib.h>

#ifdef __cplusplus
#define ULIGHT_NOEXCEPT noexcept
#else
#define ULIGHT_NOEXCEPT
#endif

// Ensure that we can use nullptr, even in C prior to C23.
#if !defined(__cplusplus) && !defined(nullptr)                                                     \
    && (!defined(__STDC_VERSION__) || __STDC_VERSION__ <= 201710)
/* -Wundef is avoided by using short circuiting in the condition */
#define nullptr ((void*)0)
#endif

#if defined(__cplusplus) || (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L)
#define ULIGHT_DEPRECATED [[deprecated]]
#else
#define ULIGHT_DEPRECATED
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ulight_string_view {
    const char* text;
    size_t length;
} ulight_string_view;

// LANGUAGES
// =================================================================================================

enum {
    /// @brief The amount of unique languages supported,
    /// including `ULIGHT_LANG_NONE`.
    ULIGHT_LANG_COUNT = 8
};

/// @brief A language supported by ulight for syntax highlighting.
typedef enum ulight_lang {
    /// @brief C.
    ULIGHT_LANG_C = 6,
    /// @brief C++.
    ULIGHT_LANG_CPP = 2,
    /// @brief CSS.
    ULIGHT_LANG_CSS = 5,
    /// @brief HTML.
    ULIGHT_LANG_HTML = 4,
    /// @brief JavaScript.
    ULIGHT_LANG_JS = 7,
    /// @brief Lua.
    ULIGHT_LANG_LUA = 3,
    /// @brief MMML (Missing Middle Markup Language).
    ULIGHT_LANG_MMML = 1,
    /// @brief NASM (Netwise Assembler)
    ULIGHT_LANG_NASM = 7,
    /// @brief No langage (null result).
    ULIGHT_LANG_NONE = 0,
} ulight_lang;

/// @brief Returns the `ulight_lang` whose name matches `name` exactly,
/// or `ULIGHT_LANGUAGE_NONE` if none matches.
/// Note that all ulight language names are lower-case.
ulight_lang ulight_get_lang(const char* name, size_t name_length) ULIGHT_NOEXCEPT;

typedef struct ulight_lang_entry {
    /// @brief An ASCII-encoded, lower-case name of the language.
    const char* name;
    /// @brief The length of `name`, in code units.
    size_t name_length;
    /// @brief The language.
    ulight_lang lang;
} ulight_lang_entry;

/// @brief An array of `ulight_lang_entry` containing the list of languages supported by ulight.
/// The entries are ordered lexicographically by `name`.
extern const ulight_lang_entry ulight_lang_list[];
/// @brief The size of `ulight_lang_list`, in elements.
extern const size_t ulight_lang_list_length;

/// @brief An array of "display names" of languages,
/// indexed by the values of `ulight_lang`.
///
/// For example, `ulight_lang_display_names[ULIGHT_LANG_CPP]` is `"C++"`.
/// Each `ulight_string_view` in this array is null-terminated.
extern const ulight_string_view ulight_lang_display_names[ULIGHT_LANG_COUNT];

// STATUS AND FLAGS
// =================================================================================================

/// @brief A status code for ulight operations,
/// indicating success (`ULIGHT_STATUS_OK`) or some kind of failure.
typedef enum ulight_status {
    /// @brief Syntax highlighting completed successfully.
    ULIGHT_STATUS_OK,
    /// @brief An output buffer wasn't set up properly.
    ULIGHT_STATUS_BAD_BUFFER,
    /// @brief The provided `ulight_lang` is invalid.
    ULIGHT_STATUS_BAD_LANG,
    /// @brief The given source code is not correctly UTF-8 encoded.
    ULIGHT_STATUS_BAD_TEXT,
    /// @brief Something else is wrong with the `ulight_state` that isn't described by one of
    /// the
    /// above.
    ULIGHT_STATUS_BAD_STATE,
    /// @brief Syntax highlighting was not possible because the code is malformed.
    /// This is currently not emitted by any of the syntax highlighters,
    /// but reserved as a future possible error.
    ULIGHT_STATUS_BAD_CODE,
    /// @brief Allocation failed somewhere during syntax highlighting.
    ULIGHT_STATUS_BAD_ALLOC,
    /// @brief Something went wrong that is not described by any of the other statuses.
    ULIGHT_STATUS_INTERNAL_ERROR,
} ulight_status;

typedef enum ulight_flag {
    /// @brief No flags.
    ULIGHT_NO_FLAGS = 0,
    /// @brief Merge adjacent tokens with the same highlighting.
    /// For example, two adjacent operators (e.g. `+`) with `ULIGHT_HL_SYM_OP` would get combined
    /// into a single one.
    ///
    /// While this produces more compact output,
    /// it prevents certain forms of styling from functioning correctly.
    /// For example, if we ultimately produce HTML and want to put a border around each operator,
    /// this would falsely result in blocks of operators appearing as one.
    ///
    /// By default, every token in the language is treated as a separate span.
    /// For example, the `<<` operator in C++ results in one `ULIGHT_HL_SYM_OP` token,
    /// but `{}` results in two `ULIGHT_HL_SYM_BRACE` tokens.
    ULIGHT_COALESCE = 1,
    /// @brief Adhere as strictly to the most recent language specification as possible.
    /// For example, this means that C keywords or compiler extension keywords in C++
    /// do not get highlighted.
    ///
    /// By default, ulight "over-extends" its highlighting a bit to provide better UX,
    /// rather than maximizing conformance.
    ULIGHT_STRICT = 2,
} ulight_flag;

// TOKENS
// =================================================================================================

typedef enum ulight_highlight_type {
    // 0x00..0x0f Meta-highlighting
    // -------------------------------------------------------------------------

    /// @brief A malformed but recognized construct,
    /// such as `0b155` in C++,
    /// which is recognized as a binary literal in C++, but which has digits
    /// that are not valid for a binary literal.
    ULIGHT_HL_ERROR = 0x00,

    // 0x00..0x2f Common highlighting
    // -------------------------------------------------------------------------

    /// @brief In all languages, comment content in general.
    ULIGHT_HL_COMMENT = 0x10,
    /// @brief In all languages, the delimiting characters of a comment.
    /// For example, `//` for line comments in C++.
    ULIGHT_HL_COMMENT_DELIM = 0x11,
    /// @brief In all languages, a builtin constant, value, literal, etc. in general.
    ULIGHT_HL_VALUE = 0x12,
    /// @brief In all languages, a numeric literal, like `123`.
    ULIGHT_HL_NUMBER = 0x16,
    /// @brief In all languages, delimiters or digit separators for numbers,
    /// like `'` within `1'000'000` in C++, or `_` within `1_000_000` in Java.
    ULIGHT_HL_NUMBER_DELIM = 0x17,
    /// @brief In all language, prefixes, suffixes, units, etc. for numbers,
    /// like `10em` or `10%` in CSS.
    ULIGHT_HL_NUMBER_DECOR = 0x18,
    /// @brief In all languages, a string or character literal, like `"etc"`.
    ULIGHT_HL_STRING = 0x1a,
    /// @brief In all languages, delimiters for strings, such as single or double quotes.
    ULIGHT_HL_STRING_DELIM = 0x1b,
    /// @brief In all language, string prefixes, suffixes, etc.,
    /// like `u8"abc"` or `"123"sv` in C++.
    ULIGHT_HL_STRING_DECOR = 0x1c,
    /// @brief In all languages, "escape sequences",
    /// possibly within a string literal, like `"\n"`.
    ULIGHT_HL_ESCAPE = 0x20,
    /// @brief In all languages, a `null`, `nullptr`, `undefined`, etc. keyword.
    ULIGHT_HL_NULL = 0x21,
    /// @brief In all languages, a `true`, `false`, `yes`, `no`, etc. keyword.
    ULIGHT_HL_BOOL = 0x22,
    /// @brief In all languages, a self-referential keyword, like `this` or `self`.
    ULIGHT_HL_THIS = 0x23,

    // 0x30..0x3f Preprocessing, macros, etc.
    // -------------------------------------------------------------------------
    ULIGHT_HL_MACRO = 0x30,

    // 0x40..0x7f Coding languages
    // -------------------------------------------------------------------------

    /// @brief In coding languages, identifiers of various programming constructs.
    ULIGHT_HL_ID = 0x40,
    /// @brief In coding languages,
    /// an identifier in a declaration.
    ULIGHT_HL_ID_DECL = 0x41,
    /// @brief In coding languages, a variable identifier.
    ULIGHT_HL_ID_VAR = 0x42,
    /// @brief In coding languages,
    /// an identifier in a variable declaration, like `int x;`.
    ULIGHT_HL_ID_VAR_DECL = 0x43,
    /// @brief In coding languages, a constant identifier.
    ULIGHT_HL_ID_CONST = 0x44,
    /// @brief In coding languages,
    /// an identifier in a constant declaration,
    /// like `const int x;`.
    ULIGHT_HL_ID_CONST_DECL = 0x45,
    /// @brief In coding languages, a function identifier, like `f()`.
    ULIGHT_HL_ID_FUNCTION = 0x46,
    /// @brief In coding languages,
    /// an identifier in a function declaration,
    /// like `void f()`.
    ULIGHT_HL_ID_FUNCTION_DECL = 0x47,
    /// @brief In coding languages, a type (alias) identifier.
    ULIGHT_HL_ID_TYPE = 0x4a,
    /// @brief In coding languages,
    /// an identifier in the declaration of a type or type alias,
    /// like `class C`.
    ULIGHT_HL_ID_TYPE_DECL = 0x4b,
    /// @brief In coding languages,
    /// an identifier in of a module, namespace, or other such construct.
    ULIGHT_HL_ID_MODULE = 0x4c,
    /// @brief In coding languages,
    /// an identifier in the declaration of a module, namespace, or other such construct.
    ULIGHT_HL_ID_MODULE_DECL = 0x4d,
    /// @brief In coding language, a label identifier,
    /// like in `goto label`.
    ULIGHT_HL_ID_LABEL = 0x4e,
    /// @brief In coding languages,
    /// an identifier in the declaration of a label,
    /// like in `label: while (true)`.
    ULIGHT_HL_ID_LABEL_DECL = 0x4f,

    /// @brief In coding languages, a keyword, like `import`.
    ULIGHT_HL_KEYWORD = 0x50,
    /// @brief In coding languages, a keyword that directs control flow, like `if`.
    ULIGHT_HL_KEYWORD_CONTROL = 0x51,
    /// @brief In coding languages, a keyword that specifies a type, like `int`.
    ULIGHT_HL_KEYWORD_TYPE = 0x52,
    /// @brief In coding languages, a keyword that represents an operator, like `and`.
    ULIGHT_HL_KEYWORD_OP = 0x53,

    /// @brief In languages with attribute syntax, the attribute content
    ULIGHT_HL_ATTR = 0x60,
    /// @brief In languages with attribute syntax, the attribute delimiters
    ULIGHT_HL_ATTR_DELIM = 0x61,

    // 0x80..0x8f Unidiff highlighting
    // -------------------------------------------------------------------------

    /// @brief In unidiff, a heading (`--- from-file`, `+++ to-file`).
    ULIGHT_HL_DIFF_HEADING = 0x80,
    /// @brief In unidiff, a common (unmodified) line.
    ULIGHT_HL_DIFF_COMMON = 0x81,
    /// @brief In unidiff, a hunk heading (`@@ ... @@`)
    ULIGHT_HL_DIFF_HUNK = 0x82,
    /// @brief In unidiff, a deletion line.
    ULIGHT_HL_DIFF_DELETION = 0x83,
    /// @brief In unidiff, an insertion line.
    ULIGHT_HL_DIFF_INSERTION = 0x84,

    // 0x90..0xaf Markup-specific highlighting
    // -------------------------------------------------------------------------

    /// @brief In Markup languages, a tag, like the name of `html` in `<html>`.
    ULIGHT_HL_MARKUP_TAG = 0x90,
    /// @brief In Markup languages, the name of an attribute.
    ULIGHT_HL_MARKUP_ATTR = 0x91,

    /// @brief In Markup languages, deleted text.
    ULIGHT_HL_MARKUP_DELETION = 0x98,
    /// @brief In Markup languages, inserted text.
    ULIGHT_HL_MARKUP_INSERTION = 0x99,

    // Note that the following 15 values use a bitmask,
    // where the first 4 bits are always 1010, followed by:
    //  - 1-bit if emphasized
    //  - 2-bit if strong
    //  - 4-bit if underlined
    //  - 8-bit if strikethrough

    /// @brief DO NOT EMIT. Helper base value only.
    ULIGHT_HL_MARKUP_TEXT = 0xa0,
    /// @brief In Markup languages, emphasized text.
    ULIGHT_HL_MARKUP_EMPH = 0xa1,
    /// @brief In Markup languages, strong text.
    ULIGHT_HL_MARKUP_STRONG = 0xa2,
    /// @brief In Markup languages, emphasized, strong text.
    ULIGHT_HL_MARKUP_EMPH_STRONG = 0xa3,
    /// @brief In Markup languages, underlined text.
    ULIGHT_HL_MARKUP_UNDERLINE = 0xa4,
    /// @brief In Markup languages, emphasized, underlined text.
    ULIGHT_HL_MARKUP_EMPH_UNDERLINE = 0xa5,
    /// @brief In Markup languages, strong, underlined text.
    ULIGHT_HL_MARKUP_STRONG_UNDERLINE = 0xa6,
    /// @brief In Markup languages, strong, underlined text.
    ULIGHT_HL_MARKUP_EMPH_STRONG_UNDERLINE = 0xa7,
    /// @brief In Markup languages, strikethrough text.
    ULIGHT_HL_MARKUP_STRIKETHROUGH = 0xa8,
    /// @brief In Markup languages, emphasized, strikethrough text.
    ULIGHT_HL_MARKUP_EMPH_STRIKETHROUGH = 0xa9,
    /// @brief In Markup languages, strong, strikethrough text.
    ULIGHT_HL_MARKUP_STRONG_STRIKETHROUGH = 0xaa,
    /// @brief In Markup languages, emphasized, strong, strikethrough text.
    ULIGHT_HL_MARKUP_EMPH_STRONG_STRIKETHROUGH = 0xab,
    /// @brief In Markup languages, underlined, strikethrough text.
    ULIGHT_HL_MARKUP_UNDERLINE_STRIKETHROUGH = 0xac,
    /// @brief In Markup languages, emphasized, underlined, strikethrough text.
    ULIGHT_HL_MARKUP_EMPH_UNDERLINE_STRIKETHROUGH = 0xad,
    /// @brief In Markup languages, underlined, strong, strikethrough text.
    ULIGHT_HL_MARKUP_STRONG_UNDERLINE_STRIKETHROUGH = 0xae,
    /// @brief In Markup languages, emphasized, strong, underlined, strikethrough text.
    ULIGHT_HL_MARKUP_EMPH_STRONG_UNDERLINE_STRIKETHROUGH = 0xaf,

    // 0xc0..0xcf Symbols with special meaning
    // -------------------------------------------------------------------------

    /// @brief In languages where a symbol has special meaning, a special symbol in general.
    ULIGHT_HL_SYM = 0xc0,
    /// @brief  Punctuation such as commas and semicolons that separate other content,
    /// and which are of no great significance.
    /// For example, this includes commas and semicolons in C,
    /// but does not include commas in Markup languages, where they have no special meaning.
    ULIGHT_HL_SYM_PUNC = 0xc1,
    /// @brief Parentheses in languages where they have special meaning,
    /// such as parentheses in C++ function calls or declarations.
    ULIGHT_HL_SYM_PARENS = 0xc4,
    /// @brief Square brackets in languages where they have special meaning,
    /// such as square brackets in C++ subscript.
    ULIGHT_HL_SYM_SQUARE = 0xc5,
    /// @brief Braces in languages where they have special meaning,
    /// such as braces in C++ class declarations, or in TeX commands.
    ULIGHT_HL_SYM_BRACE = 0xc6,
    /// @brief Operators like `+` in languages where they have special meaning.
    ULIGHT_HL_SYM_OP = 0xc7,

} ulight_highlight_type;

/// @brief Returns a textual representation made of ASCII characters and underscores of `type`.
/// This is used as a value in `ulight_tokens_to_html`.
ULIGHT_DEPRECATED
ulight_string_view ulight_highlight_type_id(ulight_highlight_type type) ULIGHT_NOEXCEPT;

ulight_string_view ulight_highlight_type_long_string(ulight_highlight_type type) ULIGHT_NOEXCEPT;

ulight_string_view ulight_highlight_type_short_string(ulight_highlight_type type) ULIGHT_NOEXCEPT;

typedef struct ulight_token {
    /// @brief The index of the first code point within the source code that has the highlighting.
    size_t begin;
    /// @brief The length of the token, in code points.
    size_t length;
    /// @brief The type of highlighting applied to the token.
    unsigned char type;
} ulight_token;

// MEMORY MANAGEMENT
// =================================================================================================

/// @brief Allocates `size` bytes with `size` alignment,
/// and returns a pointer to the allocated storage.
/// If allocation fails, returns null.
void* ulight_alloc(size_t size, size_t alignment) ULIGHT_NOEXCEPT;

/// @brief Frees memory previously allocated with `ulight_alloc`.
/// The `size` and `alignment` parameters have to be the same as the arguments
/// passed to `ulight_alloc`.
void ulight_free(void* pointer, size_t size, size_t alignment) ULIGHT_NOEXCEPT;

// STATE AND HIGHLIGHTING
// =================================================================================================

/// @brief Holds state for all functionality that ulight provides.
/// Instances of ulight should be initialized using `ulight_init` (see below),
/// and destroyed using `ulight_destroy`.
/// Otherwise, there is no guarantee that resources won't be leaked.
typedef struct ulight_state {
    /// @brief A pointer to UTF-8 encoded source code to be highlighted.
    /// `source` does not need to be null-terminated.
    const char* source;
    /// @brief The length of the UTF-8 source code, in code units,
    /// not including a potential null terminator at the end.
    size_t source_length;
    /// @brief  The language to use for syntax highlighting.
    ulight_lang lang;
    /// Set of flags, obtained by combining named `ulight_flag` entries with `|`.
    ulight_flag flags;

    /// @brief A buffer of tokens provided by the user.
    ulight_token* token_buffer;
    /// @brief The length of `token_buffer`.
    size_t token_buffer_length;
    /// @brief  Passed as the first argument into `flush_tokens`.
    void* flush_tokens_data;
    /// @brief When `token_buffer` is full,
    /// is invoked with `flush_tokens_data`, `token_buffer`, and `token_buffer_length`.
    void (*flush_tokens)(void*, ulight_token*, size_t);

    /// @brief For HTML generation, the UTF-8-encoded name of tags.
    const char* html_tag_name;
    /// @brief For HTML generation, the length of tag names, in code units.
    size_t html_tag_name_length;
    /// @brief For HTML generation, the UTF-8-encoded name of attributes.
    const char* html_attr_name;
    /// @brief For HTML generation, the length of attribute names, in code units.
    size_t html_attr_name_length;

    /// @brief A buffer for the UTF-8-encoded HTML output.
    char* text_buffer;
    /// @brief The length of `text_buffer`.
    size_t text_buffer_length;
    /// @brief Passed as the first argument into `flush_text`.
    void* flush_text_data;
    /// @brief When `text_buffer` is full,
    /// is invoked with `flush_text_data`, `text_buffer`, and `text_buffer_length`.
    void (*flush_text)(void*, char*, size_t);

    /// @brief A brief UTF-8-encoded error text.
    const char* error;
    /// @brief The length of `error`, in code units.
    size_t error_length;
} ulight_state;

///  @brief "Default constructor" for `ulight_state`.
ulight_state* ulight_init(ulight_state* state) ULIGHT_NOEXCEPT;

/// @brief "Destructor" for `ulight_state`.
void ulight_destroy(ulight_state* state) ULIGHT_NOEXCEPT;

/// @brief Allocates a `struct ulight` object using `ulight_alloc`,
/// and initializes it using `ulight_init`.
///
/// Note that dynamic allocation of `struct ulight` isn't necessary,
/// but this function may be helpful for use in WASM.
ulight_state* ulight_new(void) ULIGHT_NOEXCEPT;

/// @brief Frees a `struct ulight` object previously returned from `ulight_new`.
void ulight_delete(ulight_state* state) ULIGHT_NOEXCEPT;

/// @brief Converts the given UTF-8-encoded code in range
///`[state->source, state->source + state->source_length)` into an array of tokens,
/// written to the token buffer.
///
/// The token buffer is pointed to by `state->token_buffer`,
/// has length `state->token_buffer_length`,
/// and both these members are provided by the caller.
/// Whenever the buffer is full, `state->flush_tokens` is invoked,
/// which is also user-provided.
/// It is the caller's responsibility to store the tokens permanently if they want to,
/// such as in a `std::vector` in C++.
ulight_status ulight_source_to_tokens(ulight_state* state) ULIGHT_NOEXCEPT;

/// @brief Converts the given UTF-8-encoded code in range
///`[state->source, state->source + state->source_length)` into HTML,
/// written to text buffer.
/// Additionally, the user has to provide a token buffer for intermediate conversions.
///
/// The token buffer is pointed to by `state->token_buffer`
/// and has length `state->token_buffer_length`.
/// The text buffer is pointed to by `state->text_buffer`
/// and length `state->text_buffer_length`.
/// All these are provided by the user.
///
/// Whenever the text buffer is full, `state->flush_text` is invoked.
/// `state->flush_tokens` is automatically set.
ulight_status ulight_source_to_html(ulight_state* state) ULIGHT_NOEXCEPT;

#ifdef __cplusplus
}
#endif

// NOLINTEND
#endif
