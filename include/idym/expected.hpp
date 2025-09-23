#ifndef IDYM_EXPECTED_H
#define IDYM_EXPECTED_H

#include <exception>
#include <initializer_list>

#include "utility.hpp"
#include "functional.hpp"
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
IDYM_INTERNAL_CXX17_INLINE constexpr unexpect_t unexpect{};

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

// === swappable_base
template<typename T, typename E>
constexpr bool expected_swappable_v =
    is_swappable_v<T> && is_swappable_v<E> &&
    ::std::is_move_constructible<T>::value && ::std::is_move_constructible<E>::value &&
    (::std::is_nothrow_move_constructible<T>::value || ::std::is_nothrow_move_constructible<E>::value);

template<typename T, typename E>
constexpr bool expected_swap_noexcept_v =
    ::std::is_nothrow_move_constructible<T>::value && is_nothrow_swappable_v<T> &&
    ::std::is_nothrow_move_constructible<E>::value && is_nothrow_swappable_v<E>;

template<typename T, typename E>
IDYM_INTERNAL_CXX20_CONSTEXPR_TRYCATCH void swap_expected(::std::true_type, expected_base<T, E>& this_ref, expected_base<T, E>& rhs) {
    E tmp(::std::move(rhs._unex));
    rhs._unex.~E();

    try {
        ::new (::std::addressof(rhs._val)) T(::std::move(this_ref._val));
        this_ref._val.~T();
        ::new (::std::addressof(this_ref._unex)) E(::std::move(tmp));
    } catch (...) {
        ::new (::std::addressof(rhs._unex)) E(::std::move(tmp));
        throw;
    }
}
template<typename T, typename E>
IDYM_INTERNAL_CXX20_CONSTEXPR_TRYCATCH void swap_expected(::std::false_type, expected_base<T, E>& this_ref, expected_base<T, E>& rhs) {
    T tmp(::std::move(this_ref._val));
    this_ref._val.~T();

    try {
        ::new (::std::addressof(this_ref._unex)) E(::std::move(rhs._unex));
        rhs._unex.~E();
        ::new (::std::addressof(rhs._val)) T(::std::move(tmp));
    } catch (...) {
        ::new (::std::addressof(this_ref._val)) T(::std::move(tmp));
        throw;
    }
}

template<bool, typename, typename, typename>
struct swappable_base_impl;

template<typename T, typename E, typename Expected_T>
struct swappable_base_impl<true, T, E, Expected_T> {
    constexpr void swap(Expected_T& rhs) noexcept(expected_swap_noexcept_v<T, E>) {
        auto& this_expected = static_cast<Expected_T&>(*this);

        if (rhs._has_value && this_expected._has_value) {
            using ::std::swap;
            swap(this_expected._val, rhs._val);
            return;
        }
        if (rhs._has_value && !this_expected._has_value) {
            rhs.swap(this_expected);
            return;
        }
        if (!rhs._has_value && !this_expected._has_value) {
            using ::std::swap;
            swap(this_expected._unex, rhs._unex);
            return;
        }

        swap_expected(::std::integral_constant<bool, ::std::is_nothrow_move_constructible<E>::value>{}, this_expected, rhs);
        this_expected._has_val = false;
        rhs._has_val = true;
    }

    friend constexpr void swap(Expected_T& x, Expected_T& y) noexcept(expected_swap_noexcept_v<T, E>) {
        x.swap(y);
    }
};

template<typename T, typename E, typename Expected_T>
struct swappable_base_impl<false, T, E, Expected_T> {};

template<typename T, typename E, typename Expected_T>
using swappable_base = swappable_base_impl<expected_swappable_v<T, E>, T, E, Expected_T>;

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
class expected : _internal::expected_def_ctor_base<T, E>, public _internal::swappable_base<T, E, expected<T, E>> {
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

    // unexpected ass traits
    template<typename GF>
    static constexpr auto unexpected_ass_constraint_v =
        ::std::is_constructible<E, GF>::value &&
        ::std::is_assignable<E&, GF>::value &&
        (::std::is_nothrow_constructible<E, GF>::value || ::std::is_nothrow_move_constructible<T>::value || ::std::is_nothrow_move_constructible<E>::value);

public:
    using value_type = T;
    using error_type = E;
    using unexpected_type = unexpected<E>;

    // === ctors
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

    template<
        typename G,
        ::std::enable_if_t<!::std::is_convertible<const G&, E>::value && ::std::is_constructible<E, const G&>::value, bool> = true
    >
    constexpr explicit expected(const unexpected<G>& e) : expected{_internal::dummy_t{}} {
        forward_construct_e(e.error());
    }

    template<
        typename G,
        ::std::enable_if_t<::std::is_convertible<const G&, E>::value && ::std::is_constructible<E, const G&>::value, bool> = true
    >
    constexpr expected(const unexpected<G>& e) : expected{_internal::dummy_t{}} {
        forward_construct_e(e.error());
    }

    template<
        typename G,
        ::std::enable_if_t<!::std::is_convertible<G, E>::value && ::std::is_constructible<E, G>::value, bool> = true
    >
    constexpr explicit expected(unexpected<G>&& e) : expected{_internal::dummy_t{}} {
        forward_construct_e(::std::move(e.error()));
    }

    template<
        typename G,
        ::std::enable_if_t<::std::is_convertible<G, E>::value && ::std::is_constructible<E, G>::value, bool> = true
    >
    constexpr expected(unexpected<G>&& e) : expected{_internal::dummy_t{}} {
        forward_construct_e(::std::move(e.error()));
    }

    template<
        typename... Args,
        ::std::enable_if_t<::std::is_constructible<T, Args...>::value, bool> = true
    >
    constexpr explicit expected(in_place_t, Args&&... args) : expected{_internal::dummy_t{}} {
        forward_construct_t(::std::forward<Args>(args)...);
    }

    template<
        typename U, typename... Args,
        ::std::enable_if_t<::std::is_constructible<T, ::std::initializer_list<U>&, Args...>::value, bool> = true
    >
    constexpr explicit expected(in_place_t, ::std::initializer_list<U> il, Args&&... args) : expected{_internal::dummy_t{}} {
        forward_construct_t(il, ::std::forward<Args>(args)...);
    }

    template<
        typename... Args,
        ::std::enable_if_t<::std::is_constructible<E, Args...>::value, bool> = true
    >
    constexpr explicit expected(unexpect_t, Args&&... args) : expected{_internal::dummy_t{}} {
        forward_construct_e(::std::forward<Args>(args)...);
    }

    template<
        typename U, typename... Args,
        ::std::enable_if_t<::std::is_constructible<E, ::std::initializer_list<U>&, Args...>::value, bool> = true
    >
    constexpr explicit expected(unexpect_t, ::std::initializer_list<U> il, Args&&... args) : expected{_internal::dummy_t{}} {
        forward_construct_e(il, ::std::forward<Args>(args)...);
    }

    // === assignment
    template<
        typename U = ::std::remove_cv_t<T>,
        ::std::enable_if_t<
            !::std::is_same<expected, remove_cvref_t<U>>::value &&
            !_internal::is_specialization_of_v<remove_cvref_t<U>, unexpected> &&
            ::std::is_constructible<T, U>::value &&
            ::std::is_assignable<T&, U>::value &&
            (::std::is_nothrow_constructible<T, U>::value || ::std::is_nothrow_move_constructible<T>::value || ::std::is_nothrow_move_constructible<E>::value),
        bool> = true
    >
    constexpr expected& operator=(U&& v) {
        if (this->_has_value) {
            this->_val = ::std::forward<U>(v);
        } else {
            _internal::reinit_expected(this->_val, this->_unex, ::std::forward<U>(v));
            this->_has_val = true;
        }
        return *this;
    }

    template<typename G, ::std::enable_if_t<unexpected_ass_constraint_v<const G&>, bool> = true>
    constexpr expected& operator=(const unexpected<G>& e) {
        return forward_assign_e(e.error());
    }

    template<typename G, ::std::enable_if_t<unexpected_ass_constraint_v<G>, bool> = true>
    constexpr expected& operator=(unexpected<G>&& e) {
        return forward_assign_e(::std::move(e.error()));
    }

    // === emplace
    template<typename... Args, ::std::enable_if_t<::std::is_nothrow_constructible<T, Args...>::value, bool> = true>
    constexpr T& emplace(Args&&... args) noexcept {
        return emplace_impl(::std::forward<Args>(args)...);
    }

    template<typename U, typename... Args, ::std::enable_if_t<::std::is_nothrow_constructible<T, ::std::initializer_list<U>&, Args...>::value, bool> = true>
    constexpr T& emplace(::std::initializer_list<U> il, Args&&... args) noexcept {
        return emplace_impl(il, ::std::forward<Args>(args)...);
    }

    // === observers
    constexpr const T* operator->() const noexcept {
        return ::std::addressof(this->_val);
    }
    constexpr T* operator->() noexcept {
        return ::std::addressof(this->_val);
    }

    constexpr const T& operator*() const & noexcept {
        return this->_val;
    }
    constexpr T& operator*() & noexcept {
        return this->_val;
    }

    constexpr T&& operator*() && noexcept {
        return ::std::move(this->_val);
    }
    constexpr const T&& operator*() const && noexcept {
        return ::std::move(this->_val);
    }

    constexpr explicit operator bool() const noexcept {
        return this->_has_val;
    }
    constexpr bool has_value() const noexcept {
        return this->_has_val;
    }

    constexpr const T& value() const & {
        if (this->_has_value)
            return this->_val;
        throw bad_expected_access<E>(as_const(this->_unex));
    }
    constexpr T& value() & {
        if (this->_has_value)
            return this->_val;
        throw bad_expected_access<E>(as_const(this->_unex));
    }

    constexpr T&& value() && {
        if (this->_has_value)
            return ::std::move(this->_val);
        throw bad_expected_access<E>(::std::move(this->_unex));
    }
    constexpr const T&& value() const && {
        if (this->_has_value)
            return ::std::move(this->_val);
        throw bad_expected_access<E>(::std::move(this->_unex));
    }

    constexpr const E& error() const & noexcept {
        return this->_unex;
    }
    constexpr E& error() & noexcept {
        return this->_unex;
    }

    constexpr E&& error() && noexcept {
        return ::std::move(this->_unex);
    }
    constexpr const E&& error() const && noexcept {
        return ::std::move(this->_unex);
    }

    template<typename U = remove_cvref_t<T>>
    constexpr T value_or(U&& v) const & {
        return this->_has_value ? **this : static_cast<T>(::std::forward<U>(v));
    }
    template<typename U = remove_cvref_t<T>>
    constexpr T value_or(U&& v)  && {
        return this->_has_value ? ::std::move(**this) : static_cast<T>(::std::forward<U>(v));
    }

    template<typename G = E>
    constexpr E error_or(G&& e) const & {
        if (this->_has_value)
            return ::std::forward<G>(e);
        return error();
    }
    template<typename G = E>
    constexpr E error_or(G&& e) && {
        if (this->_has_value)
            return ::std::forward<G>(e);
        return ::std::move(error());
    }

    // === monads
    template<typename F, ::std::enable_if_t<::std::is_constructible<E, E&>::value, bool> = true>
    constexpr auto and_then(F&& f) & {
        if (this->_has_value)
            return invoke(::std::forward<F>(f), this->_val);
        return remove_cvref_t<decltype(invoke(::std::forward<F>(f), this->_val))>(unexpect, error());
    }
    template<typename F, ::std::enable_if_t<::std::is_constructible<E, const E&>::value, bool> = true>
    constexpr auto and_then(F&& f) const & {
        if (this->_has_value)
            return invoke(::std::forward<F>(f), this->_val);
        return remove_cvref_t<decltype(invoke(::std::forward<F>(f), this->_val))>(unexpect, error());
    }

    template<typename F, ::std::enable_if_t<::std::is_constructible<E, E&&>::value, bool> = true>
    constexpr auto and_then(F&& f) && {
        if (this->_has_value)
            return invoke(::std::forward<F>(f), ::std::move(this->_val));
        return remove_cvref_t<decltype(invoke(::std::forward<F>(f), ::std::move(this->_val)))>(unexpect, ::std::move(error()));
    }
    template<typename F, ::std::enable_if_t<::std::is_constructible<E, const E&&>::value, bool> = true>
    constexpr auto and_then(F&& f) const && {
        if (this->_has_value)
            return invoke(::std::forward<F>(f), ::std::move(this->_val));
        return remove_cvref_t<decltype(invoke(::std::forward<F>(f), ::std::move(this->_val)))>(unexpect, ::std::move(error()));
    }

    template<typename F, ::std::enable_if_t<::std::is_constructible<T, T&>::value, bool> = true>
    constexpr auto or_else(F&& f) & {
        if (this->_has_value)
            return remove_cvref_t<decltype(invoke(::std::forward<F>(f), this->_unex))>(in_place, this->_val);
        return invoke(::std::forward<F>(f), this->_unex);
    }
    template<typename F, ::std::enable_if_t<::std::is_constructible<T, const T&>::value, bool> = true>
    constexpr auto or_else(F&& f) const & {
        if (this->_has_value)
            return remove_cvref_t<decltype(invoke(::std::forward<F>(f), this->_unex))>(in_place, this->_val);
        return invoke(::std::forward<F>(f), this->_unex);
    }

    template<typename F, ::std::enable_if_t<::std::is_constructible<T, T&&>::value, bool> = true>
    constexpr auto or_else(F&& f) && {
        if (this->_has_value)
            return remove_cvref_t<decltype(invoke(::std::forward<F>(f), ::std::move(this->_unex)))>(in_place, ::std::move(this->_val));
        return invoke(::std::forward<F>(f), ::std::move(this->_unex));
    }
    template<typename F, ::std::enable_if_t<::std::is_constructible<T, const T&&>::value, bool> = true>
    constexpr auto or_else(F&& f) && {
        if (this->_has_value)
            return remove_cvref_t<decltype(invoke(::std::forward<F>(f), ::std::move(this->_unex)))>(in_place, ::std::move(this->_val));
        return invoke(::std::forward<F>(f), ::std::move(this->_unex));
    }

private:
    friend _internal::swappable_base<T, E, expected>;

    constexpr expected(_internal::dummy_t) : _internal::expected_def_ctor_base<T, E>{_internal::dummy_t{}} {}

    template<typename Expected_T>
    constexpr void forward_construct_compat_expected(Expected_T&& other) {
        if (other.has_value())
            ::new (::std::addressof(this->_val)) T(*::std::forward<Expected_T>(other));
        else
            ::new (::std::addressof(this->_unex)) E(::std::forward<Expected_T>(other).error());
        this->_has_value = other.has_value();
    }

    template<typename... Us>
    constexpr void forward_construct_t(Us&&... us) {
        ::new (::std::addressof(this->_val)) T(::std::forward<Us>(us)...);
        this->_has_value = true;
    }
    template<typename... Gs>
    constexpr void forward_construct_e(Gs&&... gs) {
        ::new (::std::addressof(this->_unex)) E(::std::forward<Gs>(gs)...);
        this->_has_value = false;
    }

    template<typename G>
    constexpr expected& forward_assign_e(G&& g) {
        if (this->_has_value) {
            _internal::reinit_expected(this->_unex, this->_val, ::std::forward<G>(g));
            this->_has_val = false;
        } else {
            this->_unex = ::std::forward<G>(g);
        }
        return *this;
    }

    template<typename... Ts>
    constexpr T& emplace_impl(Ts&&... args) noexcept {
        if (this->_has_val) {
            this->_val.~T();
        } else {
            this->_unex.~E();
            this->_has_val = true;
        }

        ::new (::std::addressof(this->_val)) T(::std::forward<Ts>(args)...);
        return this->_val;
    }
};

}

#endif
