#ifndef IDYM_FUNCTIONAL_H
#define IDYM_FUNCTIONAL_H

#include <functional>
#include <type_traits>

#include "type_traits.hpp"

namespace IDYM_NAMESPACE {

namespace _internal { // >>> internal
template<class>
constexpr bool is_reference_wrapper_v = false;
template<class U>
constexpr bool is_reference_wrapper_v<::std::reference_wrapper<U>> = true;

// function case
template<typename Member, typename Object, typename... Args>
constexpr decltype(auto) invoke_member(::std::true_type, ::std::integral_constant<::std::size_t, 0>, Member member, Object&& object, Args&&... args) {
    return (::std::forward<Object>(object).*member)(::std::forward<Args>(args)...);
}
template<typename Member, typename Object, typename... Args>
constexpr decltype(auto) invoke_member(::std::true_type, ::std::integral_constant<::std::size_t, 1>, Member member, Object&& object, Args&&... args) {
    return (object.get().*member)(::std::forward<Args>(args)...);
}
template<typename Member, typename Object, typename... Args>
constexpr decltype(auto) invoke_member(::std::true_type, ::std::integral_constant<::std::size_t, 2>, Member member, Object&& object, Args&&... args) {
    return ((*::std::forward<Object>(object)).*member)(::std::forward<Args>(args)...);
}
// variable case
template<typename Member, typename Object>
constexpr decltype(auto) invoke_member(::std::false_type, ::std::integral_constant<::std::size_t, 0>, Member member, Object&& object) {
    return ::std::forward<Object>(object).*member;
}
template<typename Member, typename Object>
constexpr decltype(auto) invoke_member(::std::false_type, ::std::integral_constant<::std::size_t, 1>, Member member, Object&& object) {
    return object.get().*member;
}
template<typename Member, typename Object>
constexpr decltype(auto) invoke_member(::std::false_type, ::std::integral_constant<::std::size_t, 2>, Member member, Object&& object) {
    return (*::std::forward<Object>(object)).*member;
}

template<typename C, typename Pointed, typename Object, typename... Args>
constexpr decltype(auto) invoke2(::std::true_type, Pointed C::* member, Object&& object, Args&&... args) {
    using object_t = remove_cvref_t<Object>;
    constexpr bool is_wrapped = is_reference_wrapper_v<object_t>;
    constexpr bool is_derived_object = ::std::is_same<C, object_t>::value || ::std::is_base_of<C, object_t>::value;
    constexpr ::std::size_t dispatch_ind = is_derived_object ? 0 : (is_wrapped ? 1 : 2);

    return invoke_member(
        ::std::is_function<Pointed>{}, ::std::integral_constant<::std::size_t, dispatch_ind>{},
        member, ::std::forward<Object>(object), ::std::forward<Args>(args)...
    );
}
template<typename F, typename... Args>
constexpr decltype(auto) invoke2(::std::false_type, F&& f, Args&&... args) {
    return ::std::forward<F>(f)(::std::forward<Args>(args)...);
}

} // <<< internal

// === invoke
template<typename F, typename... Args>
constexpr decltype(auto) invoke(F&& f, Args&&... args) {
    return _internal::invoke2(::std::is_member_pointer<remove_cvref_t<F>>{}, ::std::forward<F>(f), ::std::forward<Args>(args)...);
}

}

#endif
