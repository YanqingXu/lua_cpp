#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <utility>

namespace Lua {
    // Signed integers
    using i8 = int8_t;
    using i16 = int16_t;
    using i32 = int32_t;
    using i64 = int64_t;

    // Unsigned integers
    using u8 = uint8_t;
    using u16 = uint16_t;
    using u32 = uint32_t;
    using u64 = uint64_t;

    // Floating point
    using f32 = float;
    using f64 = double;

    // Size and difference types
    using usize = size_t;
    using isize = ptrdiff_t;

    // String types
    using Str = std::string;

    // Container templates
    template<typename T>
    using Vec = std::vector<T>;

    template<typename K, typename V>
    using HashMap = std::unordered_map<K, V>;

    // Smart pointers
    template<typename T>
    using Ptr = std::shared_ptr<T>;

    template<typename T>
    using WeakPtr = std::weak_ptr<T>;

    template<typename T>
    using UniquePtr = std::unique_ptr<T>;

    // Create a shared_ptr
    template<typename T, typename... Args>
    Ptr<T> make_ptr(Args&&... args) {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }

    // Create a unique_ptr
    template<typename T, typename... Args>
    UniquePtr<T> make_unique(Args&&... args) {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }
}