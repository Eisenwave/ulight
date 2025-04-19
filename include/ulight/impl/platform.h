#ifndef ULIGHT_PLATFORM_HPP
#define ULIGHT_PLATFORM_HPP

#ifdef __cplusplus
#if __cplusplus >= 202302L
#define ULIGHT_CPP 2023
#endif
#else
#define ULIGHT_CPP 1998
#endif

#ifdef __EMSCRIPTEN__
#define ULIGHT_EMSCRIPTEN 1
#define ULIGHT_IF_EMSCRIPTEN(...) __VA_ARGS__
#else
#define ULIGHT_IF_EMSCRIPTEN(...)
#endif

#ifdef __clang__
#define ULIGHT_CLANG __clang__
#endif

#if defined(__GNUC__) && !defined(__clang__)
#define ULIGHT_GCC __GNUC__
#endif

#ifdef _MSC_VER
#define ULIGHT_MSVC _MSC_VER
#endif

#if defined(ULIGHT_CPP23) && __has_cpp_attribute(assume)
#define ULIGHT_ASSUME(...) [[assume(__VA_ARGS__)]]
#elif defined(__clang__)
#define ULIGHT_ASSUME(...) __builtin_assume(__VA_ARGS__)
#else
#define ULIGHT_ASSUME(...)
#endif

#ifdef _MSC_VER
#define ULIGHT_UNREACHABLE() __assume(false)
#else
#define ULIGHT_UNREACHABLE() __builtin_unreachable()
#endif

#ifdef __cplusplus
namespace ulight {

/// @brief The default underlying type for scoped enumerations.
using Underlying = unsigned char;

} // namespace ulight
#endif

#ifdef ULIGHT_EMSCRIPTEN
#include <emscripten.h>
#define ULIGHT_EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define ULIGHT_EXPORT
#endif

#ifdef ULIGHT_GCC
#define ULIGHT_SUPPRESS_MISSING_DECLARATIONS_WARNING()                                             \
    _Pragma("GCC diagnostic ignored \"-Wmissing-declarations\"")
#else
#define ULIGHT_SUPPRESS_MISSING_DECLARATIONS_WARNING()
#endif

#endif
