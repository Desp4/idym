#ifndef IDYM_IDYM_DEFS_H
#define IDYM_IDYM_DEFS_H

#ifndef IDYM_NAMESPACE
  #define IDYM_NAMESPACE idym
#endif

#if __cpp_inline_variables >= 201606L
  #define IDYM_INTERNAL_CXX17_INLINE inline
#else
  #define IDYM_INTERNAL_CXX17_INLINE
#endif

#if __cpp_constexpr >= 201907L
  #define IDYM_INTERNAL_CXX20_CONSTEXPR_VIRTUAL constexpr
#else
  #define IDYM_INTERNAL_CXX20_CONSTEXPR_VIRTUAL
#endif

#if __cpp_constexpr >= 202002L
  #define IDYM_INTERNAL_CXX20_CONSTEXPR_DTOR constexpr
#else
  #define IDYM_INTERNAL_CXX20_CONSTEXPR_DTOR
#endif

namespace IDYM_NAMESPACE {
namespace _internal {
struct dummy_t {};
}
}

#endif
