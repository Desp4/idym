#ifndef IDYM_TYPE_TRAITS_H
#define IDYM_TYPE_TRAITS_H

#include <type_traits>

#include "idym_defs.hpp"

namespace IDYM_NAMESPACE {

// === conjunction
template<typename...>
struct conjunction : ::std::true_type {};
template<typename T>
struct conjunction<T> : T {};

template<typename T, typename... Ts>
struct conjunction<T, Ts...> : ::std::conditional_t<static_cast<bool>(T::value), conjunction<Ts...>, T> {};

template<typename... Ts>
constexpr bool conjunction_v = conjunction<Ts...>::value;

namespace _internal { // >>> internal

// === make_void
template<typename...>
struct make_void {
    using type = void;
};

namespace _std_sandbox { // >>> std sandbox

using namespace ::std;

// === swappable_with
template<typename T, typename U, typename = void>
struct swappable_with : ::std::false_type {};
template<typename T, typename U>
struct swappable_with<
    T, U,
    typename ::IDYM_NAMESPACE::_internal::make_void<decltype(swap(::std::declval<T>(), ::std::declval<U>()))>::type
> : ::std::true_type {};

// nothrow_swappable
template<typename T, typename U>
struct nothrow_swappable : ::std::integral_constant<bool,
    noexcept(swap(::std::declval<T>(), ::std::declval<U>())) && noexcept(swap(::std::declval<U>(), ::std::declval<T>()))
> {};
} // <<< std sandbox

template<typename T, typename U>
struct swappable_with_2 : ::std::integral_constant<bool, conjunction_v<_std_sandbox::swappable_with<T, U>, _std_sandbox::swappable_with<U, T>>> {};
template<typename T, typename U>
struct nothrow_swappable_2 : ::std::integral_constant<bool, conjunction_v<swappable_with_2<T, U>, _std_sandbox::nothrow_swappable<T, U>>> {};

} // <<< internal

// === remove_cvref
template<typename T>
struct remove_cvref {
    using type = ::std::remove_cv_t<::std::remove_reference_t<T>>;
};
template<typename T>
using remove_cvref_t = typename remove_cvref<T>::type;

// ===  void_t
template<typename... Ts>
using void_t = typename _internal::make_void<Ts...>::type;

// === is_swappable_with
template<typename T, typename U>
struct is_swappable_with : _internal::swappable_with_2<T, U> {};
template<typename T, typename U>
constexpr bool is_swappable_with_v = is_swappable_with<T, U>::value;

// === is_swappable
template<typename T>
struct is_swappable : is_swappable_with<::std::add_lvalue_reference_t<T>, ::std::add_lvalue_reference_t<T>> {};
template<typename T>
constexpr bool is_swappable_v = is_swappable<T>::value;

// === is_nothrow_swappable_with
template<typename T, typename U>
struct is_nothrow_swappable_with : _internal::nothrow_swappable_2<T, U> {};
template<typename T, typename U>
constexpr bool is_nothrow_swappable_with_v = is_nothrow_swappable_with<T, U>::value;

// === is_nothrow_swappable
template<typename T>
struct is_nothrow_swappable : is_nothrow_swappable_with<::std::add_lvalue_reference_t<T>, ::std::add_lvalue_reference_t<T>> {};
template<typename T>
constexpr bool is_nothrow_swappable_v = is_nothrow_swappable<T>::value;


}

#endif
