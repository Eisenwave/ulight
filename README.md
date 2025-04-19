![CMake build status][badge-cmake]
![Emscripten build and deploy to pages][badge-em]
![clang-format build status][badge-format]

# µlight
µlight or "u-light" is a zero-dependency, lightweight, and portable syntax highlighter.

## Usage

µlight provides a C API for the library,
as well as a C++ wrapper for that API.
Here is a minimal example using the C++ API:
```cpp
#include <iostream>
#include <string_view>
#include "ulight/ulight.hpp"

int main() {
    ulight::Token token_buffer[1024];
    char text_buffer[8192];

    ulight::State state;
    state.set_source("int x;\n");
    state.set_lang(ulight::Lang::cpp);
    state.set_token_buffer(token_buffer);
    state.set_text_buffer(text_buffer);
    state.on_flush_text([](const char* str, std::size_t length) {
        std::cout << std::string_view(str, length);
    });
    state.source_to_html();
    // (In practice, check the returned status from source_to_html)
}
```
This code outputs:
```html
<h- data-h=kw_type>int</h-> <h- data-h=id>x</h-><h- data-h=sym_punc>;</h->
```

For more details,
check the [examples](https://github.com/Eisenwave/ulight/tree/main/examples).

## Building from source

Building µlight from source is fairly simple.
There are no (required) external dependencies,
so you only need a relatively recent version of CMake and a C++ compiler.
To build with CMake, run:

```sh
# The compiler specified is just an example; you can also build with GCC.
cmake -B build \
    -DCMAKE_CXX_COMPILER=clang++-19 \
    -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

While µlight is written in C++23 and requires a fairly recent compiler to build,
you can use the library with an ancient compiler and C++98, once built.
It can also be used with any version of C, including C89.

That is because the library's API is exposed through the `ulight.h` header,
which can be compiled in any version of C and C++,
and because the library's ABI is C-based.
The `ulight.hpp` header is just a wrapper for convenience,
and requires C++20 to use.

### Building the WASM library

µlight is also designed to target WASM, using emscripten.
Building the WASM library also enables you to use the live editor under `www/`.

To build WASM, first
[install emsdk](https://emscripten.org/docs/getting_started/downloads.html).
`install` and `activate` the SDK tools.

Then, build using CMake:
```sh
# The actual toolchain file path depends on where you've installed emsdk.
cmake -B build \
    -DCMAKE_TOOLCHAIN_FILE=emsdk/emscripten/cmake/Modules/Platform/Emscripten.cmake \
    -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

This will output the `ulight.wasm` WebAssembly module under `build/`.
It also copies everything necessary into `www/`.
You can now use the live editor by hosting that directory as a static website.
This can be done easily if you have Python installed, like:
```sh
cd www
python -m http.server
```
Open `http://localhost:8000/` in your browser.

### Build Requirements

- CMake 3.24 or greater, and
- GCC 13 or greater, or
- Clang 19 or greater, or
- Emscripten 3.1.53 or greater.

## Language support

µlight is still in its early stages, so not a lot of languages are supported.
Every supported language has a "display name",
one ore more "short names",
and a stable numeric ID whose value can be found in `include/ulight.h`.

| Display name | Short names | `ulight_lang` |
| ------------ | ----------- | ------------- |
| Bash | `bash`, `sh`, `zsh` | `ULIGHT_LANG_BASH` |
| C | `c`, `h` | `ULIGHT_LANG_C` |
| C++ | `c++`, `cc`, `cplusplus`, `cpp`, `cxx`, `h++`, `hpp` | `ULIGHT_LANG_CPP` |
| CSS | `css` | `ULIGHT_LANG_CSS` |
| Diff | `diff`, `patch` | `ULIGHT_LANG_DIFF` |
| HTML | `htm`, `html` | `ULIGHT_LANG_HTML` |
| JavaScript | `javascript`, `js`, `jsx` | `ULIGHT_LANG_JS` |
| Lua | `lua` | `ULIGHT_LANG_LUA` |
| MMML | `mmml` | `ULIGHT_LANG_MMML` |

The long-term plan is to get support for at least 100 languages.
This may sound like a lot, but considering that many are similar to one another (e.g. JavaScript/TypeScript, XML/HTML),
this should be achievable.

### Queue

If you want to contribute any new language, feel welcome to do so.
However, some have higher priority than others,
based on how frequently they're used and other metrics.
One such metric is [Octoverse 2024](https://github.blog/news-insights/octoverse/octoverse-2024/).

You can check the open issues in this repository for planned languages.

[badge-cmake]: https://github.com/eisenwave/ulight/actions/workflows/cmake-multi-platform.yml/badge.svg
[badge-em]: https://github.com/Eisenwave/ulight/actions/workflows/pages.yml/badge.svg
[badge-format]: https://github.com/eisenwave/ulight/actions/workflows/clang-format.yml/badge.svg
