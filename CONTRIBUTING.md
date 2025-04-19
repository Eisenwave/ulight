# Contributing

Before you contribute, you should familiarize yourself with the µlight project structure.
In summary, µlight is a zero-dependency library written in C++ and exposing a C API,
so that many other langauges can use it (through WASM, JNI, etc.).

For testing, the native (non-WASM) verison of µlight is built using CMake,
and tests are run using Google Test, which is found via the system's package manager.

## Contributing a new language

To contribute highlighting for a new language,
you should look at past pull requests that have done so,
so you know which files have to be changed.

The bare minimum set of changes for any new language includes:
- Registering a new `ulight_lang` and bumping `ULIGHT_LANG_COUNT` in `include/ulight/ulight.h`.
- Adding the corresponding entry to `ulight::Lang` in `include/ulight/ulight.hpp`
- Adding short names (such as `cpp`) to `ulight_lang_list` in `ulight.cpp`.
- Adding a display name (such as `C++`) to `ulight_lang_display_names`.
- Adding a `highlight_xxx` function in `highlight.hpp`, and dispatching to it in `highlight`.
- Creating a new set of header and source files for highlighting the language.
- Implementing the syntax highlighting.
  In essence, `std::u8string_view` goes in, `ulight::Token`s come out.
- Adding highlighting test files under `test/highlight`,
  which are pairs of source files (e.g. `x.cpp`) and expected highlight files (e.g. `x.cpp.html`).

### Highlighting numbers

In the basic case, this can be very simple.
You get `123`, and turn this into a single `ulight::Token` of type `number` (`ULIGHT_HL_NUMBER`).
However, programming languages often allow numbers to have separators, suffixes, and other parts.
Find below a table that explains how to highlight each part.

| Part | Policy | Sample input | Sample Tokens |
| ---- | ------ | ------------ | ------------- |
| Digits | Highlight as `number` | `123` | `number(123)`
| Digit separators | Only separator as `number_delim` | `1_000` (Java) | `number(1)`, `number_delim(_)`, `number(000)`
| Radix points | Radix point as `number_delim` | `0.5` | `number(0)`, `number_delim(.)`, `number(5)`
| Base attachments | Whole attachment as `number_decor` | `0xff` (C) | `number_decor(0x)`, `number(ff)`
| Unit/type suffixes etc. | Whole suffix as `number_decor` | `100em` (CSS) | `number(100)`, `number_decor(em)`
| Exponents | Exponent separator as `number_delim` | `1E-5` | `number(1)`, `number_delim(E)`, `number(-5)`
| Signs | As `number`, but usually separate | `-1` | `number(-1)` 

Note that signs like `+` and `-` are usually not part of the number token,
but are treated as unary operators in many languages.
If so, they should be highlighted as operators.

Here is a C++ example that shows off every feature combined:
```cpp
+1'000.0E-5f
```
```asm
sym_op(+)       ; unary operator, not part of the token
number(1)       ; plain digits
number_delim(') ; digit separator
number(000)     ; plain digits
number_delim(.) ; radix point
number(0)       ; plain digits
number_delim(E) ; exponent separator
number(-5)      ; digits with sign
number_decor(f) ; type/unit suffix
```

### Highlighting strings

Similar to numbers, strings consist of multiple parts,
which should be highlighted as shown below.
Also note that we don't treat character literals and string literals separately,
even if some languages (e.g. C++) do.

| Part | Policy | Sample input | Sample Tokens |
| ---- | ------ | ------------ | ------------- |
| String contents | As `string` | `"abc"` | `string_delim(")`, `string(abc)`, `string_delim(")`
| String delimiters | As `string_delim` | (see above) | (see above)
| String prefixes and suffixes | As `string_decor` | `u8"..."sv` | `string_decor(u8)`, `string_delim(")`,<br>`string(...)`, `string_delim(")`, `string_decor(sv)`
| Escape sequences | As `escape` | `"abc\n"` | `string_delim(")`, `string(abc)`,<br>`escape(\n)`, `string_delim(")`
| Basic interpolations | All as `escape` | `"Hello, $NAME"` | `string_delim(")`, `string(Hello, )`,<br>`escape($NAME)`, `string_delim(")`
| Delimited interpolations | Delimiters as `escape`,<br>rest as code | `"${100}"` | `string_delim(")`, `escape(${)`, `number(100)`,<br> `escape(})`, `string_delim(")`

Note that there are also escape sequences that contain a numeric or string value of variable length.
For example, C++ supports `\U{123ABC}` Unicode escapes containing a hex digit sequence.
Just like the fixed-length escapes like `\u1234`, the whole escape should be highlighted as `escape`.
The same applies to named character references such as `\N{EQUALS SIGN}`.

### Character testing

As part of writing a new highlighter,
you will almost always need to test whether characters fall into certain categories,
like testing if a character is an ASCII digit (`0` through `9`).
All such tests generally go into `chars.hpp`.

If there already is a general-purpose character test that does what you want,
like `is_ascii_digit`,
then use the existing function.
This only applies to the general-purpose ASCII and Unicode tests.

Otherwise, if you need more language-specific tests,
create a function for each such tests.
All functions in `chars.hpp` are *total* and *exhaustive*,
and essentially define a set.
For example, `is_ascii_digit(char32_t)` defines the set of ASCII digits.
It is `true` for every ASCII-digit,
and `false` for every non-digit.

#### Overloads for `char8_t` and `char32_t`

Every character test should have overloads for `char8_t` and `char32_t`,
although one of these may be deleted.

- If a character test can be performed using just a UTF-8 code unit,
  like `is_ascii_digit`, then both overloads should exist.
- Otherwise, if the test is only meaningful for whole code units,
  the `char8_t` overload should be deleted.
  For example, there is `is_cpp_identifier_start(char8_t) = delete`
  because there are non-ASCII code points,
  so the `char8_t` overload wouldn't be exhaustive.

Yet another variation of this is defining an extra overload set,
like `is_cpp_ascii_identifier_start(char8_t | char32_t)` which is exhaustive.

### Highlighting

## Code style

Furthermore, follow these guidelines:

1.  All source files should be auto-formatted with clang-format
    based on the project's `.clang-format` configuration.
    You can use `scripts/check-format.sh` to check whether the source files are correctly formatted,
    assuming `clang-format-19` is in your path.
    GitHub actions will also verify this.

2.  No clang-tidy checks (see `.clang-tidy`) or compiler warnings should be triggered.
    We only check clang-tidy warnings before releases.
    However, ideally, you should use `scripts/check-tidy.sh` to check for warnings,
    assuming that `clang-tidy-19` is in your path
    and that CMake produces `build/compile_commands.json`.

3.  Try to follow the project code style in general.

4.  Do not use any temporary data structures or memory allocation in general, unless necessary.
    If you do need allocations, use polymorphic containers using the given `std::pmr::memory_resource*` in your highlighter.

### Variable style

Only use `auto` static type deduction if the type is obvious or irrelevant (`auto x = c.begin()`, `auto y = int(z)`, etc.),
or if it can be inferred from within the same function:
```cpp
std::u8string_view get_string();

void f() {
    long_integer_type x = 0;
    auto x_sqr = x * x; // OK

    auto z = get_string(); // NOT OK: Type information is found elsewhere,
                           //         and this could be std::string etc.
    auto i = z.begin();    // OK: This obviously returns an iterator,
                           //     and we don't gain anything by spelling out its name. 
}
```

Furthermore, use `const` for local variables (but not for function parameters) whenver possible.
While this makes the code a bit more verbose,
it makes the few mutable variables in the project stand out and receive the attention they deserve.

### Naming conventions

- Types should always be `Upper_Snake_Case`, except in C-interoperable code like `ulight.h`.
- Anything else (concepts, variables, functions, etc.) should be `lower_snake_case`.
- The rules above also apply to template parameters.
- Macros and unscoped enumerations (`enum`, not `enum struct`) should be `SCREAM_CASE`.
- Never use the `class` keyword, always `typename`, `struct`, `enum struct`, etc.
