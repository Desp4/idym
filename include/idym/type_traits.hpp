#ifndef IDYM_TYPE_TRAITS_H
#define IDYM_TYPE_TRAITS_H

#include <functional>
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

// === disjunction
template<typename...>
struct disjunction : ::std::false_type {};
template<typename T>
struct disjunction<T> : T {};

template<typename T, typename... Ts>
struct disjunction<T, Ts...> : ::std::conditional_t<static_cast<bool>(T::value), T, disjunction<Ts...>> {};

template<typename... Ts>
constexpr bool disjunction_v = disjunction<Ts...>::value;

// === remove_cvref
template<typename T>
struct remove_cvref {
    using type = ::std::remove_cv_t<::std::remove_reference_t<T>>;
};
template<typename T>
using remove_cvref_t = typename remove_cvref<T>::type;

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

// === invoke impl
template<class>
constexpr bool is_reference_wrapper_v = false;
template<class U>
constexpr bool is_reference_wrapper_v<::std::reference_wrapper<U>> = true;

enum class invoke_dispatch_tag {
    member_ref,
    member_ptr,
    ref_wrapper,
};

template<typename C, typename Pointed, typename Object>
constexpr invoke_dispatch_tag member_dispatch_tag() {
    using object_t = remove_cvref_t<Object>;
    constexpr bool is_wrapped = is_reference_wrapper_v<object_t>;
    constexpr bool is_derived_object = ::std::is_same<C, object_t>::value || ::std::is_base_of<C, object_t>::value;
    
    if (is_derived_object)
        return invoke_dispatch_tag::member_ref;
    if (is_wrapped)
        return invoke_dispatch_tag::ref_wrapper;
    return invoke_dispatch_tag::member_ptr;
}

template<invoke_dispatch_tag Tag>
using invoke_dispatch_constant = ::std::integral_constant<invoke_dispatch_tag, Tag>;
template<typename C, typename Pointed, typename Object>
using make_invoke_dispatch_tag = invoke_dispatch_constant<member_dispatch_tag<C, Pointed, Object>()>;

// function case
template<typename Member, typename Object, typename... Args>
constexpr decltype(auto) invoke_member(::std::true_type, invoke_dispatch_constant<invoke_dispatch_tag::member_ref>, Member member, Object&& object, Args&&... args)
    noexcept(noexcept((::std::forward<Object>(object).*member)(::std::forward<Args>(args)...)))
{
    return (::std::forward<Object>(object).*member)(::std::forward<Args>(args)...);
}
template<typename Member, typename Object, typename... Args>
constexpr decltype(auto) invoke_member(::std::true_type, invoke_dispatch_constant<invoke_dispatch_tag::member_ptr>, Member member, Object&& object, Args&&... args)
    noexcept(noexcept(((*::std::forward<Object>(object)).*member)(::std::forward<Args>(args)...)))
{
    return ((*::std::forward<Object>(object)).*member)(::std::forward<Args>(args)...);
}
template<typename Member, typename Object, typename... Args>
constexpr decltype(auto) invoke_member(::std::true_type, invoke_dispatch_constant<invoke_dispatch_tag::ref_wrapper>, Member member, Object&& object, Args&&... args)
    noexcept(noexcept((object.get().*member)(::std::forward<Args>(args)...)))
{
    return (object.get().*member)(::std::forward<Args>(args)...);
}

// variable case
template<typename Member, typename Object>
constexpr decltype(auto) invoke_member(::std::false_type, invoke_dispatch_constant<invoke_dispatch_tag::member_ref>, Member member, Object&& object) noexcept {
    return ::std::forward<Object>(object).*member;
}
template<typename Member, typename Object>
constexpr decltype(auto) invoke_member(::std::false_type, invoke_dispatch_constant<invoke_dispatch_tag::member_ptr>, Member member, Object&& object) noexcept {
    return (*::std::forward<Object>(object)).*member;
}
template<typename Member, typename Object>
constexpr decltype(auto) invoke_member(::std::false_type, invoke_dispatch_constant<invoke_dispatch_tag::ref_wrapper>, Member member, Object&& object) noexcept {
    return object.get().*member;
}

template<typename C, typename Pointed, typename Object, typename... Args>
constexpr decltype(auto) invoke2(::std::true_type, Pointed C::* member, Object&& object, Args&&... args)
    noexcept(noexcept(invoke_member(::std::is_function<Pointed>{}, make_invoke_dispatch_tag<C, Pointed, Object>{}, member, ::std::forward<Object>(object), ::std::forward<Args>(args)...)))
{
    return invoke_member(
        ::std::is_function<Pointed>{}, make_invoke_dispatch_tag<C, Pointed, Object>{},
        member, ::std::forward<Object>(object), ::std::forward<Args>(args)...
    );
}
template<typename F, typename... Args>
constexpr decltype(auto) invoke2(::std::false_type, F&& f, Args&&... args)
    noexcept(noexcept(::std::forward<F>(f)(::std::forward<Args>(args)...)))
{
    return ::std::forward<F>(f)(::std::forward<Args>(args)...);
}

} // <<< internal

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

// === invoke
template<typename F, typename... Args>
constexpr decltype(auto) invoke(F&& f, Args&&... args)
    noexcept(noexcept(_internal::invoke2(::std::is_member_pointer<remove_cvref_t<F>>{}, ::std::forward<F>(f), ::std::forward<Args>(args)...)))
{
    return _internal::invoke2(::std::is_member_pointer<remove_cvref_t<F>>{}, ::std::forward<F>(f), ::std::forward<Args>(args)...);
}

namespace _internal { // >>> internal

template<typename, typename, typename...>
struct invoke_result {};

template<typename F, typename... Args>
struct invoke_result<void_t<decltype(invoke(::std::declval<F>(), ::std::declval<Args>()...))>, F, Args...> {
    using type = decltype(invoke(::std::declval<F>(), ::std::declval<Args>()...));
};

} // <<< internal

// === invoke_result
template<typename F, typename... Args>
struct invoke_result : _internal::invoke_result<void, F, Args...> {};

template<typename F, typename... Args>
using invoke_result_t = typename invoke_result<F, Args...>::type;

}

#endif
