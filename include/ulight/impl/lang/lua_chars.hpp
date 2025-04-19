#ifndef ULIGHT_LUA_CHARS_HPP
#define ULIGHT_LUA_CHARS_HPP

#include "ulight/impl/ascii_chars.hpp"
#include "ulight/impl/charset.hpp"
#include "ulight/impl/unicode_chars.hpp"

namespace ulight {

inline constexpr Charset256 is_lua_whitespace_set = detail::to_charset256(u8"\t\n\f\r \v");

[[nodiscard]]
constexpr bool is_lua_whitespace(char8_t c)
{
    // > In source code, Lua recognizes as spaces the standard ASCII whitespace characters space,
    // > form feed, newline, carriage return, horizontal tab, and vertical tab.
    // https://www.lua.org/manual/5.4/manual.html,section 3.1
    return is_lua_whitespace_set.contains(c);
}

[[nodiscard]]
constexpr bool is_lua_whitespace(char32_t c)
{
    return is_ascii(c) && is_lua_whitespace(char8_t(c));
}

inline constexpr Charset256 is_lua_identifier_start_set
    = is_ascii_xid_start_set | detail::to_charset256(u8'_');

[[nodiscard]]
constexpr bool is_lua_identifier_start(char8_t c)
{
    return c == u8'_' || is_ascii_xid_start(c);
}

[[nodiscard]]
constexpr bool is_lua_identifier_start(char32_t c)
{
    // Lua identifiers start with a letter or underscore
    // See: https://www.lua.org/manual/5.4/manual.html
    return c == U'_' || is_ascii_xid_start(c);
}

inline constexpr Charset256 is_lua_identifier_continue_set = is_ascii_xid_continue_set;

[[nodiscard]]
constexpr bool is_lua_identifier_continue(char8_t c)
{
    return is_ascii_xid_continue(c);
}

[[nodiscard]]
constexpr bool is_lua_identifier_continue(char32_t c)
{
    // Lua identifiers contain letters, digits, or underscores after the first char
    return is_ascii_xid_continue(c);
}

} // namespace ulight

#endif
