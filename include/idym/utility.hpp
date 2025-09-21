#ifndef IDYM_UTILITY_H
#define IDYM_UTILITY_H

#include <cstddef>
#include <type_traits>

#include "idym_defs.hpp"

namespace IDYM_NAMESPACE {

// === in_place_t
struct in_place_t { explicit in_place_t() = default; };
// === in_place
IDYM_INTERNAL_CXX17_INLINE constexpr in_place_t in_place{};

// === in_place_type_t
template<typename T>
struct in_place_type_t { explicit in_place_type_t() = default; };
// === in_place_type
template<typename T>
constexpr in_place_type_t<T> in_place_type{};

// === in_place_index_t
template<::std::size_t I>
struct in_place_index_t { explicit in_place_index_t() = default; };
// === in_place_index
template<::std::size_t I>
constexpr in_place_index_t<I> in_place_index{};

// === as_const
template<typename T>
constexpr ::std::add_const_t<T>& as_const(T& t) noexcept {
    return t;
}
template<typename T>
void as_const(const T&&) = delete;

}

#endif
