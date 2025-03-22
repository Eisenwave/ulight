#ifndef ULIGHT_ULIGHT_H
#define ULIGHT_ULIGHT_H
// NOLINTBEGIN

#include "stdalign.h"
#include "stddef.h"
#include "stdlib.h"

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

#ifdef __cplusplus
extern "C" {
#endif

// LANGUAGES
// =================================================================================================

/// A language supported by ulight for syntax highlighting.
typedef enum ulight_lang {
    /// C++.
    ULIGHT_LANG_CPP = 2,
    /// MMML (Missing Middle Markup Language).
    ULIGHT_LANG_MMML = 1,
    /// No langage (null result).
    ULIGHT_LANG_NONE = 0,
} ulight_lang;

/// Returns the `ulight_lang` whose name matches `name` exactly,
/// or `ULIGHT_LANGUAGE_NONE` if none matches.
/// Note that all ulight language names are lower-case.
ulight_lang ulight_get_lang(const char* name, size_t name_length) ULIGHT_NOEXCEPT;

typedef struct ulight_lang_entry {
    /// An ASCII-encoded, lower-case name of the language.
    const char* name;
    /// The length of `name`, in code units.
    size_t name_length;
    /// The language.
    ulight_lang lang;
} ulight_lang_entry;

/// An array of `ulight_lang_entry` containing the list of languages supported by ulight.
/// The entries are ordered lexicographically by `name`.
extern const ulight_lang_entry ulight_lang_list[];
/// The size of `ulight_lang_list`, in elements.
extern const size_t ulight_lang_list_length;

// STATUS AND FLAGS
// =================================================================================================

typedef enum ulight_status {
    /// Syntax highlighting completed successfully.
    ULIGHT_STATUS_OK,
    /// The state wasn't set up properly.
    /// This may happen when `source` is null,
    /// so there is no data to highlight,
    /// or if `alloc_function` and `free_function` are null pointers.
    /// This status is indicative of developer mistakes.
    ULIGHT_STATUS_BAD_STATE,
    /// The given source code is not correctly UTF-8 encoded.
    ULIGHT_STATUS_BAD_TEXT,
    /// Syntax highlighting was not possible because the code is malformed.
    /// This is currently not emitted by any of the syntax highlighters,
    /// but reserved as a future possible error.
    ULIGHT_BAD_CODE,
} ulight_status;

typedef enum ulight_flag {
    /// No flags.
    ULIGHT_NO_FLAGS = 0,
    /// Merge adjacent tokens with the same highlighting.
    ULIGHT_COALESCE = 1,
    ULIGHT_STRICT = 2,
} ulight_flag;

// TOKENS
// =================================================================================================

typedef enum ulight_highlight_type {
    // 0x00..0x0f Meta-highlighting
    // -------------------------------------------------------------------------

    /// A malformed but recognized construct,
    /// such as `0b155` in C++,
    /// which is recognized as a binary literal in C++, but which has digits
    /// that are not valid for a binary literal.
    ULIGHT_HL_ERROR = 0x00,

    // 0x00..0x2f Common highlighting
    // -------------------------------------------------------------------------

    /// In all languages, comment content in general.
    ULIGHT_HL_COMMENT = 0x10,
    /// In all languages, the delimiting characters of a comment.
    /// For example, `//` for line comments in C++.
    ULIGHT_HL_COMMENT_DELIMITER = 0x01,
    /// In all languages, a builtin constant, value, literal, etc. in general.
    ULIGHT_HL_VALUE = 0x10,
    /// In all languages, a numeric literal, like `123`.
    ULIGHT_HL_NUMBER = 0x11,
    /// In all languages, a string or character literal, like `"etc"`.
    ULIGHT_HL_STRING = 0x14,
    /// In all languages, "escape sequences",
    /// possibly within a string literal, like `"\n"`.
    ULIGHT_HL_ESCAPE = 0x16,
    /// In all languages, a `null`, `nullptr`, `undefined`, etc. keyword.
    ULIGHT_HL_NULL = 0x18,
    /// In all languages, a `true`, `false`, `yes`, `no`, etc. keyword.
    ULIGHT_HL_BOOL = 0x19,
    /// In all languages, a self-referential keyword, like `this` or `self`.
    ULIGHT_HL_THIS = 0x1a,

    // 0x30..0x3f Preprocessing, macros, etc.
    // -------------------------------------------------------------------------
    ULIGHT_HL_MACRO = 0x30,

    // 0x40..0x7f Coding languages
    // -------------------------------------------------------------------------

    /// In coding languages, identifiers of various programming constructs.
    ULIGHT_HL_ID = 0x40,
    /// In coding languages,
    /// an identifier in a declaration.
    ULIGHT_HL_ID_DECL = 0x42,
    /// In coding languages,
    /// an identifier in use of a construct.
    ULIGHT_HL_ID_USE = 0x43,
    /// In coding languages,
    /// an identifier in a variable declaration, like `int x;`.
    ULIGHT_HL_ID_VAR_DECL = 0x44,
    /// In coding languages,
    /// an identifier when a variable is used.
    ULIGHT_HL_ID_VAR_USE = 0x45,
    /// In coding languages,
    /// an identifier in a constant declaration,
    /// like `const int x;`.
    ULIGHT_HL_ID_CONST_DECL = 0x46,
    /// In coding languages,
    /// an identifier in the use of a constant.
    ULIGHT_HL_ID_CONST_USE = 0x47,
    /// In coding languages,
    /// an identifier in a function declaration,
    /// like `void f()`.
    ULIGHT_HL_ID_FUNCTION_DECL = 0x48,
    /// In coding languages,
    /// an identifier in the use of a function, like `f()`.
    ULIGHT_HL_ID_FUNCTION_USE = 0x49,
    /// In coding languages,
    /// an identifier in the declaration of a type or type alias,
    /// like `class C`.
    ULIGHT_HL_ID_TYPE_DECL = 0x4a,
    /// In coding languages,
    /// an identifier in the use of a type or type alias.
    ULIGHT_HL_ID_TYPE_USE = 0x4b,
    /// In coding languages,
    /// an identifier in the declaration of a module, namespace, or other such construct.
    ULIGHT_HL_ID_MODULE_DECL = 0x4c,
    /// In coding languages,
    /// an identifier in the use of a module, namespace, or other such construct.
    ULIGHT_HL_ID_MODULE_USE = 0x4d,

    /// In coding languages, a keyword, like `import`.
    ULIGHT_HL_KEYWORD = 0x50,
    /// In coding languages, a keyword that directs control flow, like `if`.
    ULIGHT_HL_KEYWORD_CONTROL = 0x51,
    /// In coding languages, a keyword that specifies a type, like `int`.
    ULIGHT_HL_KEYWORD_TYPE = 0x52,

    // 0x80..0x8f Unidiff highlighting
    // -------------------------------------------------------------------------

    /// In unidiff, a heading (`--- from-file`, `+++ to-file`).
    ULIGHT_HL_DIFF_HEADING = 0x80,
    /// In unidiff, a common (unmodified) line.
    ULIGHT_HL_DIFF_COMMON = 0x81,
    /// In unidiff, a hunk heading (`@@ ... @@`)
    ULIGHT_HL_DIFF_HUNK = 0x82,
    /// In unidiff, a deletion line.
    ULIGHT_HL_DIFF_DELETION = 0x83,
    /// In unidiff, an insertion line.
    ULIGHT_HL_DIFF_INSERTION = 0x84,

    // 0x90..0x9f Markup-specific highlighting
    // -------------------------------------------------------------------------

    /// In Markup languages, a tag, like the name of `html` in `<html>`.
    ULIGHT_HL_MARKUP_TAG = 0x90,
    /// In Markup languages, the name of an attribute.
    ULIGHT_HL_MARKUP_ATTRIBUTE = 0x91,

    // 0xc0..0xcf Symbols with special meaning
    // -------------------------------------------------------------------------

    /// In languages where a symbol has special meaning, a special symbol in general.
    ULIGHT_HL_SYM = 0xc0,
    /// Punctuation such as commas and semicolons that separate other content,
    /// and which are of no great significance.
    /// For example, this includes commas and semicolons in C,
    /// but does not include commas in Markup languages, where they have no special meaning.
    ULIGHT_HL_SYM_PUNC = 0xc1,
    /// Parentheses in languages where they have special meaning,
    /// such as parentheses in C++ function calls or declarations.
    ULIGHT_HL_SYM_PARENS = 0xc4,
    /// Square brackets in languages where they have special meaning,
    /// such as square brackets in C++ subscript.
    ULIGHT_HL_SYM_SQUARE = 0xc5,
    /// Braces in languages where they have special meaning,
    /// such as braces in C++ class declarations, or in TeX commands.
    ULIGHT_HL_SYM_BRACE = 0xc6,
    /// Operators like `+` in languages where they have special meaning.
    ULIGHT_HL_SYM_OP,

} ulight_highlight_type;

typedef struct ulight_token {
    /// The index of the first code point within the source code that has the highlighting.
    size_t begin;
    /// The length of the token, in code points.
    size_t length;
    /// The type of highlighting applied to the token.
    ulight_highlight_type type;
} ulight_token;

// MEMORY MANAGEMENT
// =================================================================================================

/// Allocates `size` bytes with `size` alignment,
/// and returns a pointer to the allocated storage.
/// If allocation fails, returns null.
void* ulight_alloc(size_t size, size_t alignment) ULIGHT_NOEXCEPT;

/// Frees memory previously allocated with `ulight_alloc`.
/// The `size` and `alignment` parameters have to be the same as the arguments
/// passed to `ulight_alloc`.
void ulight_free(void* pointer, size_t size, size_t alignment) ULIGHT_NOEXCEPT;

// STATE AND HIGHLIGHTING
// =================================================================================================

/// Holds state for all functionality that ulight provides.
/// Instances of ulight should be initialized using `ulight_init` (see below),
/// and destroyed using `ulight_destroy`.
/// Otherwise, there is no guarantee that resources won't be leaked.
typedef struct ulight {
    /// An allocation function used to obtain memory during syntax highlighting.
    void* (*alloc_function)(size_t, size_t)ULIGHT_NOEXCEPT;
    /// A deallocation function used to free memory during syntax highlighting.
    void (*free_function)(void*, size_t, size_t) ULIGHT_NOEXCEPT;

    /// A pointer to UTF-8 encoded source code to be highlighted.
    /// `source` does not need to be null-terminated.
    const char* source;
    /// The length of the UTF-8 source code, in code units,
    /// not including a potential null terminator at the end.
    size_t source_length;
    /// The language to use for syntax highlighting.
    ulight_lang lang;
    /// Set of flags, obtained by combining named `ulight_flag` entries with `|`.
    ulight_flag flags;

    /// Once highlighting is done,
    /// a pointer to an array of tokens, allocated using `alloc_function`.
    ulight_token* tokens;
    /// Once highlighting is done,
    /// the length of the tokens array.
    size_t tokens_length;

    /// For HTML generation, the UTF-8-encoded name of tags.
    const char* html_tag_name;
    /// For HTML generation, the length of tag names, in code units.
    size_t html_tag_name_length;
    /// For HTML generation, the UTF-8-encoded name of attributes.
    const char* html_attr_name;
    /// For HTML generation, the length of attribute names, in code units.
    size_t html_attr_name_length;

    /// After HTML generation, the UTF-8 encoded output.
    char* html_output;
    /// After HTML generation, the length of the encoded output, in code units.
    size_t html_output_length;
} ulight;

/// "Default constructor" for ulight_state.
static inline ulight* ulight_init(ulight* state) ULIGHT_NOEXCEPT
{
    state->alloc_function = ulight_alloc;
    state->free_function = ulight_free;
    state->source = nullptr;
    state->source_length = 0;
    state->lang = ULIGHT_LANG_NONE;
    state->flags = ULIGHT_NO_FLAGS;
    state->tokens = nullptr;
    state->tokens_length = 0;
    state->html_tag_name = nullptr;
    state->html_tag_name_length = 0;
    state->html_attr_name = nullptr;
    state->html_attr_name_length = 0;
    state->html_output = nullptr;
    state->html_output_length = 0;
    return state;
}

/// "Destructor" for ulight_state.
static inline void ulight_destroy(ulight* state) ULIGHT_NOEXCEPT
{
    if (state->tokens) {
        const size_t bytes = state->tokens_length * sizeof(ulight_token);
        state->free_function(state->tokens, bytes, alignof(ulight_token));
    }
    if (state->html_output) {
        state->free_function(state->html_output, state->html_output_length, 1);
    }
}

/// Allocates a `struct ulight` object using `ulight_alloc`,
/// and initializes it using `ulight_init`.
///
/// Note that dynamic allocation of `struct ulight` isn't necessary,
/// but this function may be helpful for use in WASM.
static inline ulight* ulight_new(void) ULIGHT_NOEXCEPT
{
    void* result = ulight_alloc(sizeof(ulight), alignof(ulight));
    return ulight_init((ulight*)result);
}

/// Frees a `struct ulight` object previously returned from `ulight_new`.
static inline void ulight_delete(ulight* state) ULIGHT_NOEXCEPT
{
    ulight_free(state, sizeof(ulight), alignof(ulight));
}

/// Converts the given code in `state->source` into an array of tokens,
/// stored in `state->tokens`,
/// allocated using `state->alloc_function`.
ulight_status ulight_source_to_tokens(ulight* state) ULIGHT_NOEXCEPT;

/// Converts the given tokens in `state->tokens` into UTF-8 encoded HTML,
/// stored in `state->html_output`,
/// allocated using `state->alloc_function`.
ulight_status ulight_tokens_to_html(ulight* state) ULIGHT_NOEXCEPT;

/// Convenience function which runs `ulight_source_to_tokens`,
/// and upon success,
/// immediately runs `ulight_tokens_to_html`.
static inline ulight_status ulight_source_to_html(ulight* state) ULIGHT_NOEXCEPT
{
    const ulight_status to_tokens = ulight_source_to_tokens(state);
    if (to_tokens != ULIGHT_STATUS_OK) {
        return to_tokens;
    }
    return ulight_tokens_to_html(state);
}

#ifdef __cplusplus
}
#endif

// NOLINTEND
#endif
