#ifndef ULIGHT_BUFFER_HPP
#define ULIGHT_BUFFER_HPP

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <ranges>
#include <span>

#include "ulight/function_ref.hpp"

#include "ulight/impl/assert.hpp"

namespace ulight {

template <typename T>
struct Non_Owning_Buffer {
    using value_type = T;

private:
    value_type* m_buffer;
    std::size_t m_capacity;
    void* m_flush_data;
    void (*m_flush)(void*, value_type*, std::size_t);
    std::size_t m_size = 0;

public:
    [[nodiscard]]
    constexpr Non_Owning_Buffer(
        value_type* buffer,
        std::size_t capacity,
        void* flush_data,
        void (*flush)(void*, value_type*, std::size_t)
    )
        : m_buffer { buffer }
        , m_capacity { capacity }
        , m_flush_data { flush_data }
        , m_flush { flush }
    {
        ULIGHT_ASSERT(m_buffer != nullptr);
        ULIGHT_ASSERT(m_capacity != 0);
        // We deliberately have no expectations towards m_flush_data.
        ULIGHT_ASSERT(m_flush != nullptr);
    }

    [[nodiscard]]
    constexpr Non_Owning_Buffer(
        std::span<value_type> buffer,
        Function_Ref<void(value_type*, std::size_t)> flush
    )
        : Non_Owning_Buffer { buffer.data(), buffer.size(), flush.get_entity(),
                              flush.get_invoker() }
    {
    }

    /// @brief Returns the amount of elements that can be appended to the buffer before flushing.
    [[nodiscard]]
    constexpr std::size_t capacity() const noexcept
    {
        return m_capacity;
    }

    /// @brief Returns the number of elements currently in the buffer.
    /// `size() <= capacity()` is always `true`.
    [[nodiscard]]
    constexpr std::size_t size() const noexcept
    {
        return m_size;
    }

    /// @brief Equivalent to `capacity() - size()`.
    [[nodiscard]]
    constexpr std::size_t available() const noexcept
    {
        return m_capacity - m_size;
    }

    /// @brief Equivalent to `available() == 0`.
    [[nodiscard]]
    constexpr bool full() const noexcept
    {
        return m_size == m_capacity;
    }

    /// @brief Equivalent to `size() == 0`.
    [[nodiscard]]
    constexpr bool empty() const noexcept
    {
        return m_size == 0;
    }

    /// @brief Sets the size to zero.
    /// Since the buffer is not responsible for the lifetimes of the elements in the buffer,
    /// none are destroyed.
    constexpr void clear() noexcept
    {
        m_size = 0;
    }

    constexpr value_type& push_back(const value_type& e)
        requires std::is_copy_assignable_v<value_type>
    {
        if (full()) {
            flush();
        }
        return m_buffer[m_size++] = e;
    }

    constexpr value_type& push_back(value_type&& e)
        requires std::is_move_assignable_v<value_type>
    {
        if (full()) {
            flush();
        }
        return m_buffer[m_size++] = std::move(e);
    }

    template <typename... Args>
    constexpr value_type& emplace_back(Args&&... args)
        requires std::is_constructible_v<value_type, Args&&...>
        && std::is_move_assignable_v<value_type>
    {
        if (full()) {
            flush();
        }
        return m_buffer[m_size++] = value_type(std::forward<Args>(args)...);
    }

    template <std::input_iterator Iter, std::sentinel_for<Iter> Sentinel>
        requires std::convertible_to<std::iter_reference_t<Iter>, value_type>
    constexpr void append(Iter begin, Sentinel end)
    {
        for (; begin != end; ++begin) {
            push_back(*begin);
        }
    }

    template <std::random_access_iterator Iter>
        requires std::convertible_to<std::iter_reference_t<Iter>, value_type>
    constexpr void append(Iter begin, Iter end)
    {
        ULIGHT_DEBUG_ASSERT(begin <= end);
        using Diff = std::iter_difference_t<Iter>;
        while (begin != end) {
            if (full()) {
                flush();
            }
            const auto chunk_size = std::min(available(), std::size_t(end - begin));
            ULIGHT_DEBUG_ASSERT(chunk_size != 0);
            ULIGHT_DEBUG_ASSERT(begin + Diff(chunk_size) <= end);
            ULIGHT_DEBUG_ASSERT(m_size + chunk_size <= m_capacity);

            std::ranges::copy(begin, begin + Diff(chunk_size), m_buffer + m_size);
            begin += Diff(chunk_size);
            m_size += chunk_size;
        }
    }

    template <std::ranges::input_range R>
        requires std::convertible_to<std::ranges::range_reference_t<R>, value_type>
    constexpr void append_range(R&& range)
    {
        append(
            std::ranges::begin(std::forward<R>(range)), std::ranges::end(std::forward<R>(range))
        );
    }

    [[nodiscard]]
    constexpr value_type& back()
    {
        ULIGHT_DEBUG_ASSERT(!empty());
        return m_buffer[m_size - 1];
    }

    [[nodiscard]]
    constexpr const value_type& back() const
    {
        ULIGHT_DEBUG_ASSERT(!empty());
        return m_buffer[m_size - 1];
    }

    constexpr void flush()
    {
        if (m_size != 0) {
            m_flush(m_flush_data, m_buffer, m_size);
            m_size = 0;
        }
    }
};

} // namespace ulight

#endif
