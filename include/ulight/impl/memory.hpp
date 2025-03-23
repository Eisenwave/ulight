#ifndef ULIGHT_MEMORY_HPP
#define ULIGHT_MEMORY_HPP

#include <memory_resource>

#include "ulight/ulight.hpp"

namespace ulight {

/// @brief A `std::pmr::memory_resource` which uses
/// `ulight::alloc` and `ulight::free` to allocate or free memory.
struct Global_Memory_Resource final : std::pmr::memory_resource {

    [[nodiscard]]
    void* do_allocate(std::size_t bytes, std::size_t alignment) final
    {
        void* result = ulight::alloc(bytes, alignment);
        if (!result) {
            throw std::bad_alloc();
        }
        return result;
    }

    void do_deallocate(void* p, std::size_t bytes, std::size_t alignment) noexcept final
    {
        ulight::free(p, bytes, alignment);
    }

    [[nodiscard]]
    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept final
    {
        return dynamic_cast<const Global_Memory_Resource*>(&other) != nullptr;
    }
};

} // namespace ulight

#endif
