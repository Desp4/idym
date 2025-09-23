#ifndef IDYM_EXPECTED_H
#define IDYM_EXPECTED_H

#include <exception>
#include <initializer_list>

#include "utility.hpp"
#include "type_traits.hpp"

namespace IDYM_NAMESPACE {

template<typename, typename>
class expected;

// === unexpected
template<typename E>
class unexpected {
public:
    template<
        typename Err = E,
        ::std::enable_if_t<
            !::std::is_same<remove_cvref_t<Err>, unexpected>::value &&
            !::std::is_same<remove_cvref_t<Err>, in_place_t>::value &&
            ::std::is_constructible<E, Err>::value,
        bool> = true
    >
    constexpr explicit unexpected(Err&& e) : _unex(::std::forward<Err>(e)) {}
    
    template<
        typename... Args,
        ::std::enable_if_t<::std::is_constructible<E, Args...>::value, bool> = true
    >
    constexpr explicit unexpected(in_place_t, Args&&... args) : _unex(::std::forward<Args>(args)...) {}
    
    template<
        typename U, typename... Args,
        ::std::enable_if_t<::std::is_constructible<E, ::std::initializer_list<U>&, Args...>::value, bool> = true
    >
    constexpr explicit unexpected(in_place_t, ::std::initializer_list<U> il, Args&&... args) : _unex(il, std::forward<Args>(args)...) {}
    
    constexpr const E& error() const & noexcept {
        return _unex;
    }
    constexpr E& error() & noexcept {
        return _unex;
    }
    
    constexpr E&& error() && noexcept {
        return ::std::move(_unex);
    }
    constexpr const E&& error() const && noexcept {
        return ::std::move(_unex);
    }
    
    constexpr void swap(unexpected& other) noexcept(is_nothrow_swappable_v<E>) {
        using ::std::swap;
        swap(_unex, other._unex);
    }
    friend constexpr void swap(unexpected& x, unexpected& y) noexcept(noexcept(x.swap(y))) {
        x.swap(y);
    }
    
    template<typename E2>
    friend constexpr bool operator==(const unexpected& x, const unexpected<E2>& y) {
        return x.error() == y.error();
    }
#if __cpp_impl_three_way_comparison < 201907L
    // suppliment for synthesized !=
    template<typename E2>
    friend constexpr bool operator!=(const unexpected& x, const unexpected<E2>& y) {
        return !(x == y);
    }
#endif
    
private:
    E _unex;
};

template<typename>
class bad_expected_access;

// === bad_expected_access<void>
template<> class bad_expected_access<void> : public ::std::exception {
public:
    IDYM_INTERNAL_CXX20_CONSTEXPR_VIRTUAL const char* what() const noexcept override {
        return "Bad expected access. Too bad!";
    }
    
protected:
    bad_expected_access() noexcept = default;
    bad_expected_access(const bad_expected_access&) noexcept = default;
    bad_expected_access(bad_expected_access&&) noexcept = default;
    bad_expected_access& operator=(const bad_expected_access&) noexcept = default;
    bad_expected_access& operator=(bad_expected_access&&) noexcept = default;
};

// === bad_expected_access
template<typename E>
class bad_expected_access : public bad_expected_access<void> {
public:
    constexpr explicit bad_expected_access(E e) : _unex{::std::move(e)} {}
    
    constexpr const E& error() const & noexcept {
        return _unex;
    }
    constexpr E& error() & noexcept {
        return _unex;
    }
    
    constexpr E&& error() && noexcept {
        return ::std::move(_unex);
    }
    constexpr const E&& error() const && noexcept {
        return ::std::move(_unex);
    }
    
    IDYM_INTERNAL_CXX20_CONSTEXPR_VIRTUAL const char* what() const noexcept override {
        return "Bad expected access. Too bad!";
    }
    
private:
    E _unex;
};

// === unexpect_t
struct unexpect_t {
    explicit unexpect_t() = default;
};
IDYM_INTERNAL_CXX17_INLINE constexpr unexpect_t unexcpect{};

namespace _internal { // >>> internal

template<typename T, typename E>
constexpr bool expected_move_ctor_noexcept_v = ::std::is_nothrow_move_constructible<T>::value && ::std::is_nothrow_move_constructible<E>::value;
template<typename T, typename E>
constexpr bool expected_move_ass_noexcept_v =
    ::std::is_nothrow_move_assignable<T>::value && ::std::is_nothrow_move_constructible<T>::value &&
    ::std::is_nothrow_move_assignable<E>::value && ::std::is_nothrow_move_constructible<E>::value;

// === expected_base
template<bool, typename, typename>
struct expected_base_impl;

template<typename T, typename E>
struct expected_base_impl<true, T, E> {
    union {
        T _value;
        E _unex;
        union {} _dummy;
    };
    bool _has_val;
    
    constexpr expected_base_impl() noexcept : _dummy{} {}
};
template<typename T, typename E>
struct expected_base_impl<false, T, E> {
    union {
        T _value;
        E _unex;
        union {} _dummy;
    };
    bool _has_val;

    constexpr expected_base_impl() noexcept : _dummy{} {}
    IDYM_INTERNAL_CXX20_CONSTEXPR_DTOR ~expected_base_impl() {
        if (_has_val)
            _value.~T();
        else
            _unex.~E();
    }
};

template<typename T, typename E>
using expected_base = expected_base_impl<::std::is_trivially_destructible<T>::value && ::std::is_trivially_destructible<E>::value, T, E>;

// === reinit_expected
template<typename T, typename U, typename... Args>
constexpr void reinit_expected_dispatch_nothrow_move_cons(::std::true_type, T& newval, U& oldval, Args&&... args) {
    T tmp(::std::forward<Args>(args)...);
    oldval.~U();
    ::new (::std::addressof(newval)) T(::std::move(tmp));
}
template<typename T, typename U, typename... Args>
IDYM_INTERNAL_CXX20_CONSTEXPR_TRYCATCH void reinit_expected_dispatch_nothrow_move_cons(::std::false_type, T& newval, U& oldval, Args&&... args) {
    U tmp(::std::move(oldval));
    oldval.~U();

    try {
        ::new (::std::addressof(newval)) T(::std::forward<Args>(args)...);
    } catch (...) {
        ::new (::std::addressof(oldval)) U(::std::move(tmp));
        throw;
    }
}

template<typename T, typename U, typename... Args>
constexpr void reinit_expected_dispatch_nothrow_cons(::std::true_type, T& newval, U& oldval, Args&&... args) {
    oldval.~U();
    ::new (::std::addressof(newval)) T(::std::forward<Args>(args)...);
}
template<typename T, typename U, typename... Args>
constexpr void reinit_expected_dispatch_nothrow_cons(::std::false_type, T& newval, U& oldval, Args&&... args) {
    reinit_expected_dispatch_nothrow_move_cons(
        ::std::integral_constant<bool, ::std::is_nothrow_move_constructible<T>::value>{},
        newval, oldval, ::std::forward<Args>(args)...
    );
}

template<typename T, typename U, typename... Args>
constexpr void reinit_expected(T& newval, U& oldval, Args&&... args) {
    reinit_expected_dispatch_nothrow_cons(
        ::std::integral_constant<bool, ::std::is_nothrow_constructible<T, Args...>::value>{},
        newval, oldval, ::std::forward<Args>(args)...
    );
}

// === expected_ncopy_ass_base
template<typename T, typename E>
constexpr bool expected_ncopy_ass_defined_v =
    ::std::is_copy_assignable<T>::value && ::std::is_copy_constructible<T>::value &&
    ::std::is_copy_assignable<E>::value && ::std::is_copy_constructible<E>::value &&
    (::std::is_nothrow_move_constructible<T>::value || ::std::is_nothrow_move_constructible<E>::value);

template<typename T, typename E>
struct expected_ncopy_ass_base_impl : expected_base<T, E> {
    expected_ncopy_ass_base_impl& operator=(const expected_ncopy_ass_base_impl&) = delete;
};
template<typename T, typename E>
using expected_ncopy_ass_base = ::std::conditional_t<expected_ncopy_ass_defined_v<T, E>, expected_base<T, E>, expected_ncopy_ass_base_impl<T, E>>;

// === expected_nmove_ass_base
template<typename T, typename E>
constexpr bool expected_nmove_ass_defined_v =
    ::std::is_move_assignable<T>::value && ::std::is_move_constructible<T>::value &&
    ::std::is_move_assignable<E>::value && ::std::is_move_constructible<E>::value &&
    (::std::is_nothrow_move_constructible<T>::value || ::std::is_nothrow_move_constructible<E>::value);

template<typename T, typename E>
struct expected_nmove_ass_base_impl : expected_ncopy_ass_base<T, E> {
    expected_nmove_ass_base_impl& operator=(expected_nmove_ass_base_impl&&) = delete;
};
template<typename T, typename E>
using expected_nmove_ass_base = ::std::conditional_t<expected_nmove_ass_defined_v<T, E>, expected_ncopy_ass_base<T, E>, expected_nmove_ass_base_impl<T, E>>;

// === expected_movecopy_base
template<typename T, typename E>
struct expected_movecopy_base : expected_nmove_ass_base<T, E> {
    constexpr expected_movecopy_base() = default;
    constexpr expected_movecopy_base(const expected_movecopy_base&) = default;
    constexpr expected_movecopy_base(expected_movecopy_base&&) noexcept(expected_move_ctor_noexcept_v<T, E>) = default;
    constexpr expected_movecopy_base& operator=(const expected_movecopy_base&) = default;
    constexpr expected_movecopy_base& operator=(expected_movecopy_base&&) noexcept(expected_move_ass_noexcept_v<T, E>) = default;
};

// === expected_copy_ctor_base
template<typename T, typename E>
struct expected_copy_ctor_base_impl : expected_movecopy_base<T, E> {
    constexpr expected_copy_ctor_base_impl() = default;
    constexpr expected_copy_ctor_base_impl(const expected_copy_ctor_base_impl& other) {
        if (other._has_value)
            ::new (::std::addressof(this->_value)) T(other._value);
        else
            ::new (::std::addressof(this->_unex)) E(other._unex);
        this->_has_value = other.has_value;
    }

    constexpr expected_copy_ctor_base_impl& operator=(const expected_copy_ctor_base_impl&) = default;
    constexpr expected_copy_ctor_base_impl& operator=(expected_copy_ctor_base_impl&&) noexcept(expected_move_ass_noexcept_v<T, E>) = default;
};
template<typename T, typename E>
using expected_copy_ctor_base = ::std::conditional_t<
    (::std::is_copy_constructible<T>::value && ::std::is_copy_constructible<E>::value) &&
    !(::std::is_trivially_copy_constructible<T>::value && ::std::is_trivially_copy_constructible<E>::value),
    expected_copy_ctor_base_impl<T, E>, expected_movecopy_base<T, E>
>;

// === expected_move_ctor_base
template<typename T, typename E>
struct expected_move_ctor_base_impl : expected_copy_ctor_base<T, E> {
    constexpr expected_move_ctor_base_impl() = default;
    constexpr expected_move_ctor_base_impl(const expected_move_ctor_base_impl&) = default;
    constexpr expected_move_ctor_base_impl(expected_move_ctor_base_impl&& other) noexcept(expected_move_ctor_noexcept_v<T, E>) {
        if (other._has_value)
            ::new (::std::addressof(this->_value)) T(::std::move(other._value));
        else
            ::new (::std::addressof(this->_unex)) E(::std::move(other._unex));
        this->_has_value = other.has_value;
    }
    
    constexpr expected_move_ctor_base_impl& operator=(const expected_move_ctor_base_impl&) = default;
    constexpr expected_move_ctor_base_impl& operator=(expected_move_ctor_base_impl&&) noexcept(expected_move_ass_noexcept_v<T, E>) = default;
};
template<typename T, typename E>
using expected_move_ctor_base = ::std::conditional_t<
    (::std::is_move_constructible<T>::value && ::std::is_move_constructible<E>::value) &&
    !(::std::is_trivially_move_constructible<T>::value && ::std::is_trivially_move_constructible<E>::value),
    expected_move_ctor_base_impl<T, E>, expected_copy_ctor_base<T, E>
>;

// === expected_move_ass_base
template<typename T, typename E>
struct expected_move_ass_base_impl : expected_move_ctor_base<T, E> {
    constexpr expected_move_ass_base_impl() = default;
    constexpr expected_move_ass_base_impl(const expected_move_ass_base_impl&) = default;
    constexpr expected_move_ass_base_impl(expected_move_ass_base_impl&& other) noexcept(expected_move_ctor_noexcept_v<T, E>) = default;
    
    constexpr expected_move_ass_base_impl& operator=(const expected_move_ass_base_impl&) = default;
    constexpr expected_move_ass_base_impl& operator=(expected_move_ass_base_impl&& other) noexcept(expected_move_ass_noexcept_v<T, E>) {
        if (this->_has_value && other._has_value)
            this->_val = ::std::move(other._val);
        else if (this->_has_value)
            reinit_expected(this->_unex, this->_val, ::std::move(other._unex));
        else if (other._has_value)
            reinit_expected(this->_val, this->_unex, ::std::move(other._val));
        else
            this->_unex = ::std::move(other._unex);
        
        this->_has_value = other._has_value;
        return *this;
    }
};
template<typename T, typename E>
using expected_move_ass_base = ::std::conditional_t<expected_nmove_ass_defined_v<T, E>, expected_move_ass_base_impl<T, E>, expected_move_ctor_base<T, E>>;

// === expected_copy_ass_base
template<typename T, typename E>
struct expected_copy_ass_base_impl : expected_move_ass_base<T, E> {
    constexpr expected_copy_ass_base_impl() = default;
    constexpr expected_copy_ass_base_impl(const expected_copy_ass_base_impl&) = default;
    constexpr expected_copy_ass_base_impl(expected_copy_ass_base_impl&& other) noexcept(expected_move_ctor_noexcept_v<T, E>) = default;

    constexpr expected_copy_ass_base_impl& operator=(expected_copy_ass_base_impl&&) noexcept(expected_move_ass_noexcept_v<T, E>) = default;
    constexpr expected_copy_ass_base_impl& operator=(const expected_copy_ass_base_impl& other) {
        if (this->_has_value && other._has_value)
            this->_val = other._val;
        else if (this->_has_value)
            reinit_expected(this->_unex, this->_val, other._unex);
        else if (other._has_value)
            reinit_expected(this->_val, this->_unex, other._val);
        else
            this->_unex = other._unex;
        
        this->_has_value = other._has_value;
        return *this;
    }
};
template<typename T, typename E>
using expected_copy_ass_base = ::std::conditional_t<expected_ncopy_ass_defined_v<T, E>, expected_copy_ass_base_impl<T, E>, expected_move_ass_base<T, E>>;

// === expected_def_ctor_base
template<bool, typename, typename>
struct expected_def_ctor_base_impl;

template<typename T, typename E>
struct expected_def_ctor_base_impl<true, T, E> : expected_copy_ass_base<T, E> {
    constexpr expected_def_ctor_base_impl() {
        ::new (::std::addressof(this->_value)) T();
        this->_has_val = true;
    }
    constexpr expected_def_ctor_base_impl(dummy_t) {}
};
template<typename T, typename E>
struct expected_def_ctor_base_impl<false, T, E> : expected_copy_ass_base<T, E> {
    expected_def_ctor_base_impl() = delete;
    constexpr expected_def_ctor_base_impl(dummy_t) {}
};

template<typename T, typename E>
using expected_def_ctor_base = expected_def_ctor_base_impl<::std::is_default_constructible<T>::value, T, E>;

// === traits
template<typename T, template<typename...> class Template_T>
constexpr bool is_specialization_of_v = false;
template<template<typename...> class Template_T, typename... Ts>
constexpr bool is_specialization_of_v<Template_T<Ts...>, Template_T> = true;

template<typename T, typename W>
constexpr bool converts_from_any_cvref_v = disjunction_v<
    ::std::is_constructible<T, W&>, ::std::is_constructible<W&, T>,
    ::std::is_constructible<T, W>, ::std::is_convertible<W, T>,
    ::std::is_constructible<T, const W&>, ::std::is_convertible<const W&, T>,
    ::std::is_constructible<T, const W>, ::std::is_convertible<const W, T>
>;

} // <<< internal

// === expected
template<typename T, typename E>
class expected : _internal::expected_def_ctor_base<T, E> {
    // compat unexpected ctors traits
    template<typename U, typename G>
    static constexpr auto compat_expected_explicit_v = !::std::is_convertible<const U&, T>::value || !::std::is_convertible<const G&, E>::value;
    
    template<typename U, typename G>
    static constexpr auto compat_expected_constraint_v =
        ::std::is_constructible<T, const U&>::value &&
        ::std::is_constructible<E, const G&>::value &&
        (::std::is_same<bool, ::std::remove_cv_t<T>>::value || !_internal::converts_from_any_cvref_v<T, expected<U, G>>) &&
        !::std::is_constructible<unexpected<E>, expected<U, G>&>::value &&
        !::std::is_constructible<unexpected<E>, expected<U, G>>::value &&
        !::std::is_constructible<unexpected<E>, const expected<U, G>&>::value &&
        !::std::is_constructible<unexpected<E>, const expected<U, G>>::value;
    
    // compat T ctor traits
    template<typename U>
    static constexpr auto compat_t_explicit_v = ::std::is_convertible<U, T>::value;
    
    template<typename U>
    static constexpr auto compat_t_constraint_v =
        !::std::is_same<remove_cvref_t<U>, in_place_t>::value &&
        !::std::is_same<remove_cvref_t<U>, expected>::value &&
        !::std::is_same<remove_cvref_t<U>, unexpect_t>::value &&
        !_internal::is_specialization_of_v<remove_cvref_t<U>, unexpected> &&
        ::std::is_constructible<T, U>::value &&
        (!::std::is_same<bool, ::std::remove_cv_t<T>>::value || !_internal::is_specialization_of_v<remove_cvref_t<U>, expected>);

public:
    using value_type = T;
    using error_type = E;
    using unexpected_type = unexpected<E>;

    constexpr expected() = default;
    
    template<
        typename U, typename G,
        ::std::enable_if_t<compat_expected_explicit_v<U, G> && compat_expected_constraint_v<U, G>, bool> = true
    >
    constexpr explicit expected(const expected<U, G>& other) : expected{_internal::dummy_t{}} {
        forward_construct_compat_expected(other);
    }

    template<
        typename U, typename G,
        ::std::enable_if_t<!compat_expected_explicit_v<U, G> && compat_expected_constraint_v<U, G>, bool> = true
    >
    constexpr expected(const expected<U, G>& other) : expected{_internal::dummy_t{}} {
        forward_construct_compat_expected(other);
    }
    
    template<
        typename U, typename G,
        ::std::enable_if_t<compat_expected_explicit_v<U, G> && compat_expected_constraint_v<U, G>, bool> = true
    >
    constexpr explicit expected(expected<U, G>&& other) : expected{_internal::dummy_t{}} {
        forward_construct_compat_expected(::std::move(other));
    }

    template<
        typename U, typename G,
        ::std::enable_if_t<!compat_expected_explicit_v<U, G> && compat_expected_constraint_v<U, G>, bool> = true
    >
    constexpr expected(expected<U, G>&& other) : expected{_internal::dummy_t{}} {
        forward_construct_compat_expected(::std::move(other));
    }
    
    template<
        typename U = ::std::remove_cv_t<T>,
        ::std::enable_if_t<compat_t_explicit_v<U> && compat_t_constraint_v<U>, bool> = true
    >
    constexpr explicit expected(U&& v) : expected{_internal::dummy_t{}} {
        forward_construct_t(std::forward<U>(v));
    }
    template<
        typename U = ::std::remove_cv_t<T>,
        ::std::enable_if_t<!compat_t_explicit_v<U> && compat_t_constraint_v<U>, bool> = true
    >
    constexpr expected(U&& v) : expected{_internal::dummy_t{}} {
        forward_construct_t(std::forward<U>(v));
    }

private:
    constexpr expected(_internal::dummy_t) : _internal::expected_def_ctor_base<T, E>{_internal::dummy_t{}} {}

    template<typename Expected_T>
    void forward_construct_compat_expected(Expected_T&& other) {
        if (other.has_value())
            ::new (::std::addressof(this->_val)) T(*::std::forward<Expected_T>(other));
        else
            ::new (::std::addressof(this->_unex)) E(::std::forward<Expected_T>(other).error());
        this->_has_value = other.has_value();
    }

    template<typename U>
    void forward_construct_t(U&& v) {
        ::new (::std::addressof(this->_val)) T(::std::forward<U>(v));
        this->_has_value = true;
    }
};

}

#endif
