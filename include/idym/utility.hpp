#ifndef IDYM_UTILITY_H
#define IDYM_UTILITY_H

#include <cstddef>

#ifndef IDYM_NAMESPACE
  #define IDYM_NAMESPACE idym
#endif

#if __cpp_inline_variables >= 201606L
  #define IDYM_INTERNAL_CXX17_INLINE inline
#else
  #define IDYM_INTERNAL_CXX17_INLINE
#endif

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

}

#endif
