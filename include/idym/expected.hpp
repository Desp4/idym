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
    // suppliment for synthesized ops
    template<typename E2>
    friend constexpr bool operator!=(const unexpected& x, const unexpected<E2>& y) {
        return !(x == y);
    }
#endif
    
private:
    E _unex;
};

#if __cpp_deduction_guides >= 201703L
template<typename E> unexpected(E) -> unexpected<E>;
#endif

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

template<typename, typename T, template<typename...> class... T_Else>
struct void_or_traits : ::std::integral_constant<bool, conjunction_v<T_Else<T>...>> {};

template<typename T, template<typename...> class... T_Else>
struct void_or_traits<::std::enable_if_t<::std::is_void<T>::value>, T, T_Else...> : ::std::true_type {};

template<typename T, template<typename...> class... T_Else>
constexpr bool void_or_traits_v = void_or_traits<void, T, T_Else...>::value;

template<typename T, typename E>
constexpr bool expected_move_ctor_noexcept_v = void_or_traits_v<T, ::std::is_nothrow_move_constructible> && ::std::is_nothrow_move_constructible<E>::value;
template<typename T, typename E>
constexpr bool expected_move_ass_noexcept_v =
    void_or_traits_v<T, ::std::is_nothrow_move_assignable, ::std::is_nothrow_move_constructible> &&
    ::std::is_nothrow_move_assignable<E>::value && ::std::is_nothrow_move_constructible<E>::value;

// === expected_value_member
union empty_union {
    friend constexpr void swap(empty_union&, empty_union&) noexcept {}
};

template<typename T>
using expected_value_member_t = ::std::conditional_t<::std::is_void<T>::value, empty_union, T>;

// === expected_base
template<bool, typename, typename>
struct expected_base_impl;

template<typename T, typename E>
struct expected_base_impl<true, T, E> {
    union {
        expected_value_member_t<T> _val;
        E _unex;
        union {} _dummy;
    };
    bool _has_val;
    
    constexpr expected_base_impl() noexcept(::std::is_void<T>::value) : _dummy{} {}
};
template<typename T, typename E>
struct expected_base_impl<false, T, E> {
    union {
        expected_value_member_t<T> _val;
        E _unex;
        union {} _dummy;
    };
    bool _has_val;

    constexpr expected_base_impl() noexcept(::std::is_void<T>::value) : _dummy{} {}
    IDYM_INTERNAL_CXX20_CONSTEXPR_DTOR ~expected_base_impl() {
        using TT = expected_value_member_t<T>;
        if (_has_val)
            _val.~TT();
        else
            _unex.~E();
    }
};

template<typename T, typename E>
struct expected_base : protected expected_base_impl<
    void_or_traits_v<T, ::std::is_trivially_destructible> && ::std::is_trivially_destructible<E>::value, T, E
> {};

// === reinit_expected
template<typename T, typename U, typename... Args>
constexpr void reinit_expected_dispatch_nothrow_move_cons(::std::true_type, T& newval, U& oldval, Args&&... args) {
    expected_value_member_t<T> tmp(::std::forward<Args>(args)...);
    oldval.~U();
    ::new (::std::addressof(newval)) expected_value_member_t<T>(::std::move(tmp));
}
template<typename T, typename U, typename... Args>
IDYM_INTERNAL_CXX20_CONSTEXPR_TRYCATCH void reinit_expected_dispatch_nothrow_move_cons(::std::false_type, T& newval, U& oldval, Args&&... args) {
    U tmp(::std::move(oldval));
    oldval.~U();

    try {
        ::new (::std::addressof(newval)) expected_value_member_t<T>(::std::forward<Args>(args)...);
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
        ::std::integral_constant<bool, void_or_traits_v<T, ::std::is_nothrow_move_constructible>>{},
        newval, oldval, ::std::forward<Args>(args)...
    );
}

template<typename... Args>
struct unevaluted_is_nothrow_constructible {
    template<typename T>
    using type = ::std::is_nothrow_constructible<T, Args...>;
};

template<typename T, typename U, typename... Args>
constexpr void reinit_expected(T& newval, U& oldval, Args&&... args) {
    reinit_expected_dispatch_nothrow_cons(
        ::std::integral_constant<bool, void_or_traits_v<T, unevaluted_is_nothrow_constructible<Args...>::template type>>{},
        newval, oldval, ::std::forward<Args>(args)...
    );
}

// === expected_ncopy_ass_base
template<typename T, typename E>
constexpr bool expected_ncopy_ass_defined_v =
    void_or_traits_v<T, ::std::is_copy_assignable, ::std::is_copy_constructible> &&
    ::std::is_copy_assignable<E>::value && ::std::is_copy_constructible<E>::value &&
    (void_or_traits_v<T, ::std::is_nothrow_move_constructible> || ::std::is_nothrow_move_constructible<E>::value);

template<typename T, typename E>
struct expected_ncopy_ass_base_impl : expected_base<T, E> {
    expected_ncopy_ass_base_impl& operator=(const expected_ncopy_ass_base_impl&) = delete;
};
template<typename T, typename E>
using expected_ncopy_ass_base = ::std::conditional_t<expected_ncopy_ass_defined_v<T, E>, expected_base<T, E>, expected_ncopy_ass_base_impl<T, E>>;

// === expected_nmove_ass_base
template<typename T, typename E>
constexpr bool expected_nmove_ass_defined_v =
    void_or_traits_v<T, ::std::is_move_assignable, ::std::is_move_constructible> &&
    ::std::is_move_assignable<E>::value && ::std::is_move_constructible<E>::value &&
    (void_or_traits_v<T, ::std::is_nothrow_move_constructible> || ::std::is_nothrow_move_constructible<E>::value);

template<typename T, typename E>
struct expected_nmove_ass_base_impl : expected_ncopy_ass_base<T, E> {
    expected_nmove_ass_base_impl& operator=(expected_nmove_ass_base_impl&&) = delete;
};
template<typename T, typename E>
using expected_nmove_ass_base = ::std::conditional_t<expected_nmove_ass_defined_v<T, E>, expected_ncopy_ass_base<T, E>, expected_nmove_ass_base_impl<T, E>>;

// === expected_movecopy_base
template<typename T, typename E>
struct expected_movecopy_base : expected_nmove_ass_base<T, E> {
    constexpr expected_movecopy_base() noexcept(::std::is_void<T>::value) = default;
    constexpr expected_movecopy_base(const expected_movecopy_base&) = default;
    constexpr expected_movecopy_base(expected_movecopy_base&&) noexcept(expected_move_ctor_noexcept_v<T, E>) = default;
    constexpr expected_movecopy_base& operator=(const expected_movecopy_base&) = default;
    constexpr expected_movecopy_base& operator=(expected_movecopy_base&&) noexcept(expected_move_ass_noexcept_v<T, E>) = default;
};

// === expected_copy_ctor_base
template<typename T, typename E>
struct expected_copy_ctor_base_impl : expected_movecopy_base<T, E> {
    constexpr expected_copy_ctor_base_impl() noexcept(::std::is_void<T>::value) = default;
    constexpr expected_copy_ctor_base_impl(const expected_copy_ctor_base_impl& other) {
        if (other._has_val)
            ::new (::std::addressof(this->_val)) expected_value_member_t<T>(other._val);
        else
            ::new (::std::addressof(this->_unex)) E(other._unex);
        this->_has_val = other._has_val;
    }

    constexpr expected_copy_ctor_base_impl& operator=(const expected_copy_ctor_base_impl&) = default;
    constexpr expected_copy_ctor_base_impl& operator=(expected_copy_ctor_base_impl&&) noexcept(expected_move_ass_noexcept_v<T, E>) = default;
};
template<typename T, typename E>
using expected_copy_ctor_base = ::std::conditional_t<
    void_or_traits_v<T, ::std::is_copy_constructible> && ::std::is_copy_constructible<E>::value &&
    !(void_or_traits_v<T, ::std::is_trivially_copy_constructible> && ::std::is_trivially_copy_constructible<E>::value),
    expected_copy_ctor_base_impl<T, E>, expected_movecopy_base<T, E>
>;

// === expected_move_ctor_base
template<typename T, typename E>
struct expected_move_ctor_base_impl : expected_copy_ctor_base<T, E> {
    constexpr expected_move_ctor_base_impl() noexcept(::std::is_void<T>::value) = default;
    constexpr expected_move_ctor_base_impl(const expected_move_ctor_base_impl&) = default;
    constexpr expected_move_ctor_base_impl(expected_move_ctor_base_impl&& other) noexcept(expected_move_ctor_noexcept_v<T, E>) {
        if (other._has_val)
            ::new (::std::addressof(this->_val)) expected_value_member_t<T>(::std::move(other._val));
        else
            ::new (::std::addressof(this->_unex)) E(::std::move(other._unex));
        this->_has_val = other._has_val;
    }
    
    constexpr expected_move_ctor_base_impl& operator=(const expected_move_ctor_base_impl&) = default;
    constexpr expected_move_ctor_base_impl& operator=(expected_move_ctor_base_impl&&) noexcept(expected_move_ass_noexcept_v<T, E>) = default;
};
template<typename T, typename E>
using expected_move_ctor_base = ::std::conditional_t<
    void_or_traits_v<T, ::std::is_move_constructible> && ::std::is_move_constructible<E>::value &&
    !(void_or_traits_v<T, ::std::is_trivially_move_constructible> && ::std::is_trivially_move_constructible<E>::value),
    expected_move_ctor_base_impl<T, E>, expected_copy_ctor_base<T, E>
>;

// === expected_move_ass_base
template<typename T, typename E>
struct expected_move_ass_base_impl : expected_move_ctor_base<T, E> {
    constexpr expected_move_ass_base_impl() noexcept(::std::is_void<T>::value) = default;
    constexpr expected_move_ass_base_impl(const expected_move_ass_base_impl&) = default;
    constexpr expected_move_ass_base_impl(expected_move_ass_base_impl&& other) noexcept(expected_move_ctor_noexcept_v<T, E>) = default;
    
    constexpr expected_move_ass_base_impl& operator=(const expected_move_ass_base_impl&) = default;
    constexpr expected_move_ass_base_impl& operator=(expected_move_ass_base_impl&& other) noexcept(expected_move_ass_noexcept_v<T, E>) {
        if (this->_has_val && other._has_val)
            this->_val = ::std::move(other._val);
        else if (this->_has_val)
            reinit_expected(this->_unex, this->_val, ::std::move(other._unex));
        else if (other._has_val)
            reinit_expected(this->_val, this->_unex, ::std::move(other._val));
        else
            this->_unex = ::std::move(other._unex);
        
        this->_has_val = other._has_val;
        return *this;
    }
};
template<typename T, typename E>
using expected_move_ass_base = ::std::conditional_t<expected_nmove_ass_defined_v<T, E>, expected_move_ass_base_impl<T, E>, expected_move_ctor_base<T, E>>;

// === expected_copy_ass_base
template<typename T, typename E>
struct expected_copy_ass_base_impl : expected_move_ass_base<T, E> {
    constexpr expected_copy_ass_base_impl() noexcept(::std::is_void<T>::value) = default;
    constexpr expected_copy_ass_base_impl(const expected_copy_ass_base_impl&) = default;
    constexpr expected_copy_ass_base_impl(expected_copy_ass_base_impl&& other) noexcept(expected_move_ctor_noexcept_v<T, E>) = default;

    constexpr expected_copy_ass_base_impl& operator=(expected_copy_ass_base_impl&&) noexcept(expected_move_ass_noexcept_v<T, E>) = default;
    constexpr expected_copy_ass_base_impl& operator=(const expected_copy_ass_base_impl& other) {
        if (this->_has_val && other._has_val)
            this->_val = other._val;
        else if (this->_has_val)
            reinit_expected(this->_unex, this->_val, other._unex);
        else if (other._has_val)
            reinit_expected(this->_val, this->_unex, other._val);
        else
            this->_unex = other._unex;
        
        this->_has_val = other._has_val;
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
    constexpr expected_def_ctor_base_impl() noexcept(::std::is_void<T>::value) {
        ::new (::std::addressof(this->_val)) expected_value_member_t<T>();
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
using expected_def_ctor_base = expected_def_ctor_base_impl<void_or_traits_v<T, ::std::is_default_constructible>, T, E>;

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

template<typename T, typename W>
struct converts_from_any_cvref : ::std::integral_constant<bool, converts_from_any_cvref_v<T, W>> {};

template<typename T1, typename T2, typename = void>
struct expected_eq_test : ::std::false_type {};

template<typename T1, typename T2>
struct expected_eq_test<T1, T2, void_t<decltype(static_cast<bool>(::std::declval<T1>() == ::std::declval<T2>()))>> : ::std::true_type {};

template<typename T1, typename T2>
constexpr bool compare_expected_values(::std::true_type, const T1&, const T2&) {
    return true;
}
template<typename T1, typename T2>
constexpr bool compare_expected_values(::std::false_type, const T1& x, const T2& y) {
    return *x == *y;
}

template<typename F, typename T1, typename T2>
struct expected_monad_constraint {
    template<typename T>
    using unevaluated_constructible = ::std::is_constructible<T, T2>;

    using type = ::std::enable_if_t<void_or_traits_v<T1, unevaluated_constructible>, bool>;
};
template<typename F, typename T1, typename T2>
using expected_monad_constraint_t = typename expected_monad_constraint<F, T1, T2>::type;

// === swappable_base
template<typename T, typename E>
constexpr bool expected_swappable_v =
    void_or_traits_v<T, is_swappable> && is_swappable_v<E> &&
    void_or_traits_v<T, ::std::is_move_constructible> && ::std::is_move_constructible<E>::value &&
    (void_or_traits_v<T, ::std::is_nothrow_move_constructible> || ::std::is_nothrow_move_constructible<E>::value);

template<typename T, typename E>
constexpr bool expected_swap_noexcept_v =
    void_or_traits_v<T, ::std::is_nothrow_move_constructible, is_nothrow_swappable> &&
    ::std::is_nothrow_move_constructible<E>::value && is_nothrow_swappable_v<E>;

template<bool Trivial, typename T, typename E>
IDYM_INTERNAL_CXX20_CONSTEXPR_TRYCATCH void swap_expected(::std::true_type, expected_base_impl<Trivial, T, E>& this_ref, expected_base_impl<Trivial, T, E>& rhs) {
    using TT = expected_value_member_t<T>;
    E tmp(::std::move(rhs._unex));
    rhs._unex.~E();

    try {
        ::new (::std::addressof(rhs._val)) expected_value_member_t<T>(::std::move(this_ref._val));
        this_ref._val.~TT();
        ::new (::std::addressof(this_ref._unex)) E(::std::move(tmp));
    } catch (...) {
        ::new (::std::addressof(rhs._unex)) E(::std::move(tmp));
        throw;
    }
}
template<bool Trivial, typename T, typename E>
IDYM_INTERNAL_CXX20_CONSTEXPR_TRYCATCH void swap_expected(::std::false_type, expected_base_impl<Trivial, T, E>& this_ref, expected_base_impl<Trivial, T, E>& rhs) {
    using TT = expected_value_member_t<T>;
    expected_value_member_t<T> tmp(::std::move(this_ref._val));
    this_ref._val.~TT();

    try {
        ::new (::std::addressof(this_ref._unex)) E(::std::move(rhs._unex));
        rhs._unex.~E();
        ::new (::std::addressof(rhs._val)) expected_value_member_t<T>(::std::move(tmp));
    } catch (...) {
        ::new (::std::addressof(this_ref._val)) expected_value_member_t<T>(::std::move(tmp));
        throw;
    }
}

template<bool, typename T, typename E>
struct swappable_base_impl : expected_def_ctor_base<T, E> {
    using expected_def_ctor_base<T, E>::expected_def_ctor_base;
};

template<typename T, typename E>
struct swappable_base_impl<true, T, E> : expected_def_ctor_base<T, E> {
    using expected_def_ctor_base<T, E>::expected_def_ctor_base;

    constexpr void swap(expected<T, E>& rhs) noexcept(expected_swap_noexcept_v<T, E>) {
        auto& this_expected = static_cast<expected<T, E>&>(*this);

        if (rhs._has_val && this_expected._has_val) {
            using ::std::swap;
            swap(this_expected._val, rhs._val);
            return;
        }
        if (rhs._has_val && !this_expected._has_val) {
            rhs.swap(this_expected);
            return;
        }
        if (!rhs._has_val && !this_expected._has_val) {
            using ::std::swap;
            swap(this_expected._unex, rhs._unex);
            return;
        }

        swap_expected(::std::integral_constant<bool, ::std::is_nothrow_move_constructible<E>::value>{}, this_expected, rhs);
        this_expected._has_val = false;
        rhs._has_val = true;
    }

    friend constexpr void swap(expected<T, E>& x, expected<T, E>& y) noexcept(expected_swap_noexcept_v<T, E>) {
        x.swap(y);
    }
};

template<typename T, typename E>
using swappable_base = swappable_base_impl<expected_swappable_v<T, E>, T, E>;

// === monadic void compat invokers and ctors
template<typename Ret_T, typename Value_T>
constexpr auto make_monad_value_ret(::std::true_type, Value_T&&) {
    return Ret_T();
}
template<typename Ret_T, typename Value_T>
constexpr auto make_monad_value_ret(::std::false_type, Value_T&& value) {
    return Ret_T(in_place, ::std::forward<Value_T>(value));
}

template<typename F, typename Value_T>
constexpr auto make_monad_invoke_ret(::std::true_type, F&& f, Value_T&&) {
    return invoke(::std::forward<F>(f));
}
template<typename F, typename Value_T>
constexpr auto make_monad_invoke_ret(::std::false_type, F&& f, Value_T&& value) {
    return invoke(::std::forward<F>(f), ::std::forward<Value_T>(value));
}

template<typename F, typename T, typename = void>
struct expected_invoke_result {
    using type = remove_cvref_t<invoke_result_t<F, T>>;
};
template<typename F, typename T>
struct expected_invoke_result<F, T, ::std::enable_if_t<::std::is_same<remove_cvref_t<T>, empty_union>::value>> {
    using type = remove_cvref_t<invoke_result_t<F>>;
};

template<typename F, typename T>
using expected_invoke_result_t = typename expected_invoke_result<F, T>::type;

// === unevaluated void-aware traits
template<typename T, typename UF, typename = void>
struct is_expected_explicit_convertible : ::std::integral_constant<bool, ::std::is_convertible<UF, T>::value> {};

template<typename T, typename UF>
struct is_expected_explicit_convertible<T, UF, ::std::enable_if_t<::std::is_void<T>::value>> : ::std::true_type {};

template<typename T, typename U, template<typename...> class T_Else, typename = void>
struct void_tu_or_trait : ::std::integral_constant<bool, T_Else<T>::value> {};

template<typename T, typename U, template<typename...> class T_Else>
struct void_tu_or_trait<T, U, T_Else, ::std::enable_if_t<::std::is_void<T>::value || ::std::is_void<U>::value>> :
    ::std::integral_constant<bool, ::std::is_void<T>::value && ::std::is_void<U>::value>
{};

template<typename UF>
struct unevaluated_is_constructible {
    template<typename T>
    using type = ::std::is_constructible<T, UF>;
};

template<typename U, typename G>
struct unevaluated_converts_from_any_cvref {
    template<typename T>
    using type = disjunction<::std::is_same<bool, ::std::remove_cv_t<T>>, negation<converts_from_any_cvref<T, expected<U, G>>>>;
};

// === expected_toplevel_base
template<typename T, typename E>
struct expected_toplevel_base : swappable_base<T, E> {
private:
    using is_this_t_void = ::std::is_void<T>;

    // msvc 19.16.27050.0 refuses to treat constexpr vars or functions as const evaluated in sfinae contexts, macros instead
#define IDYM_COMPAT_EXPECTED_EXPLICIT_V \
    (!is_expected_explicit_convertible<T, UF>::value || !::std::is_convertible<GF, E>::value)

#define IDYM_COMPAT_EXPECTED_CONSTRAINT_V \
    (void_tu_or_trait<T, U, unevaluated_is_constructible<UF>::template type>::value && \
    ::std::is_constructible<E, GF>::value && \
    void_tu_or_trait<T, U, unevaluated_converts_from_any_cvref<U, G>::template type>::value && \
    !::std::is_constructible<unexpected<E>, expected<U, G>&>::value && \
    !::std::is_constructible<unexpected<E>, expected<U, G>>::value && \
    !::std::is_constructible<unexpected<E>, const expected<U, G>&>::value && \
    !::std::is_constructible<unexpected<E>, const expected<U, G>>::value)

public:
    using swappable_base<T, E>::swappable_base;
    
    using value_type = T;
    using error_type = E;
    using unexpected_type = unexpected<E>;
    
    template<class U>
    using rebind = expected<U, error_type>;

    constexpr expected_toplevel_base() noexcept(::std::is_void<T>::value) = default;

    // === ctors
    template<
        typename U, typename G,
        typename UF = const U,
        typename GF = const G,
        ::std::enable_if_t<IDYM_COMPAT_EXPECTED_EXPLICIT_V && IDYM_COMPAT_EXPECTED_CONSTRAINT_V, bool> = true
    >
    constexpr explicit expected_toplevel_base(const expected<U, G>& other) : expected_toplevel_base{dummy_t{}} {
        forward_construct_compat_expected(other);
    }

    template<
        typename U, typename G,
        typename UF = const U,
        typename GF = const G,
        ::std::enable_if_t<!IDYM_COMPAT_EXPECTED_EXPLICIT_V && IDYM_COMPAT_EXPECTED_CONSTRAINT_V, bool> = true
    >
    constexpr expected_toplevel_base(const expected<U, G>& other) : expected_toplevel_base{dummy_t{}} {
        forward_construct_compat_expected(other);
    }

    template<
        typename U, typename G,
        typename UF = U,
        typename GF = G,
        ::std::enable_if_t<IDYM_COMPAT_EXPECTED_EXPLICIT_V && IDYM_COMPAT_EXPECTED_CONSTRAINT_V, bool> = true
    >
    constexpr explicit expected_toplevel_base(expected<U, G>&& other) : expected_toplevel_base{dummy_t{}} {
        forward_construct_compat_expected(::std::move(other));
    }

    template<
        typename U, typename G,
        typename UF = U,
        typename GF = G,
        ::std::enable_if_t<!IDYM_COMPAT_EXPECTED_EXPLICIT_V && IDYM_COMPAT_EXPECTED_CONSTRAINT_V, bool> = true
    >
    constexpr expected_toplevel_base(expected<U, G>&& other) : expected_toplevel_base{dummy_t{}} {
        forward_construct_compat_expected(::std::move(other));
    }
    
    template<
        typename G,
        ::std::enable_if_t<!::std::is_convertible<const G&, E>::value && ::std::is_constructible<E, const G&>::value, bool> = true
    >
    constexpr explicit expected_toplevel_base(const unexpected<G>& e) : expected_toplevel_base{dummy_t{}} {
        this->forward_construct_e(e.error());
    }

    template<
        typename G,
        ::std::enable_if_t<::std::is_convertible<const G&, E>::value && ::std::is_constructible<E, const G&>::value, bool> = true
    >
    constexpr expected_toplevel_base(const unexpected<G>& e) : expected_toplevel_base{dummy_t{}} {
        this->forward_construct_e(e.error());
    }

    template<
        typename G,
        ::std::enable_if_t<!::std::is_convertible<G, E>::value && ::std::is_constructible<E, G>::value, bool> = true
    >
    constexpr explicit expected_toplevel_base(unexpected<G>&& e) : expected_toplevel_base{dummy_t{}} {
        this->forward_construct_e(::std::move(e.error()));
    }

    template<
        typename G,
        ::std::enable_if_t<::std::is_convertible<G, E>::value && ::std::is_constructible<E, G>::value, bool> = true
    >
    constexpr expected_toplevel_base(unexpected<G>&& e) : expected_toplevel_base{dummy_t{}} {
        this->forward_construct_e(::std::move(e.error()));
    }

    template<
        typename... Args,
        ::std::enable_if_t<::std::is_constructible<E, Args...>::value, bool> = true
    >
    constexpr explicit expected_toplevel_base(unexpect_t, Args&&... args) : expected_toplevel_base{dummy_t{}} {
        forward_construct_e(::std::forward<Args>(args)...);
    }

    template<
        typename U, typename... Args,
        ::std::enable_if_t<::std::is_constructible<E, ::std::initializer_list<U>&, Args...>::value, bool> = true
    >
    constexpr explicit expected_toplevel_base(unexpect_t, ::std::initializer_list<U> il, Args&&... args) : expected_toplevel_base{dummy_t{}} {
        forward_construct_e(il, ::std::forward<Args>(args)...);
    }
    
    // === observers
    constexpr explicit operator bool() const noexcept {
        return this->_has_val;
    }
    constexpr bool has_value() const noexcept {
        return this->_has_val;
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

    template<typename G = E>
    constexpr E error_or(G&& e) const & {
        if (this->_has_val)
            return ::std::forward<G>(e);
        return this->error();
    }
    template<typename G = E>
    constexpr E error_or(G&& e) && {
        if (this->_has_val)
            return ::std::forward<G>(e);
        return ::std::move(this->error());
    }
    
    // === monads
    template<typename F, _internal::expected_monad_constraint_t<F, E, E&> = true>
    constexpr auto and_then(F&& f) & {
        if (this->_has_val)
            return make_monad_invoke_ret(::std::is_void<T>{}, ::std::forward<F>(f), this->_val);
        return expected_invoke_result_t<F, decltype(this->_val)>(unexpect, this->error());
    }
    template<typename F, _internal::expected_monad_constraint_t<F, E, const E&> = true>
    constexpr auto and_then(F&& f) const & {
        if (this->_has_val)
            return make_monad_invoke_ret(::std::is_void<T>{}, ::std::forward<F>(f), this->_val);
        return expected_invoke_result_t<F, decltype(this->_val)>(unexpect, this->error());
    }

    template<typename F, _internal::expected_monad_constraint_t<F, E, E&&> = true>
    constexpr auto and_then(F&& f) && {
        if (this->_has_val)
            return make_monad_invoke_ret(::std::is_void<T>{}, ::std::forward<F>(f), ::std::move(this->_val));
        return expected_invoke_result_t<F, decltype(::std::move(this->_val))>(unexpect, ::std::move(this->error()));
    }
    template<typename F, _internal::expected_monad_constraint_t<F, E, const E&&> = true>
    constexpr auto and_then(F&& f) const && {
        if (this->_has_val)
            return make_monad_invoke_ret(::std::is_void<T>{}, ::std::forward<F>(f), ::std::move(this->_val));
        return expected_invoke_result_t<F, decltype(::std::move(this->_val))>(unexpect, ::std::move(this->error()));
    }

    template<typename F, _internal::expected_monad_constraint_t<F, T, ::std::add_lvalue_reference_t<T>> = true>
    constexpr auto or_else(F&& f) & {
        if (this->_has_val)
            return make_monad_value_ret<remove_cvref_t<invoke_result_t<F, decltype(this->error())>>>(::std::is_void<T>{}, this->_val);
        return invoke(::std::forward<F>(f), this->_unex);
    }
    template<typename F, _internal::expected_monad_constraint_t<F, T, ::std::add_lvalue_reference_t<const T>> = true>
    constexpr auto or_else(F&& f) const & {
        if (this->_has_val)
            return make_monad_value_ret<remove_cvref_t<invoke_result_t<F, decltype(this->error())>>>(::std::is_void<T>{}, this->_val);
        return invoke(::std::forward<F>(f), this->_unex);
    }

    template<typename F, _internal::expected_monad_constraint_t<F, T, ::std::add_rvalue_reference_t<T>> = true>
    constexpr auto or_else(F&& f) && {
        if (this->_has_val)
            return make_monad_value_ret<remove_cvref_t<invoke_result_t<F, decltype(::std::move(this->error()))>>>(::std::is_void<T>{}, ::std::move(this->_val));
        return invoke(::std::forward<F>(f), ::std::move(this->_unex));
    }
    template<typename F, _internal::expected_monad_constraint_t<F, T, ::std::add_rvalue_reference_t<const T>> = true>
    constexpr auto or_else(F&& f) const && {
        if (this->_has_val)
            return make_monad_value_ret<remove_cvref_t<invoke_result_t<F, decltype(::std::move(this->error()))>>>(::std::is_void<T>{}, ::std::move(this->_val));
        return invoke(::std::forward<F>(f), ::std::move(this->_unex));
    }

    template<typename F, _internal::expected_monad_constraint_t<F, E, E&> = true>
    constexpr auto transform(F&& f) & {
        return transform_impl(*this, ::std::forward<F>(f));
    }
    template<typename F, _internal::expected_monad_constraint_t<F, E, const E&> = true>
    constexpr auto transform(F&& f) const & {
        return transform_impl(*this, ::std::forward<F>(f));
    }

    template<typename F, _internal::expected_monad_constraint_t<F, E, E&&> = true>
    constexpr auto transform(F&& f) && {
        return transform_impl(::std::move(*this), ::std::forward<F>(f));
    }
    template<typename F, _internal::expected_monad_constraint_t<F, E, const E&&> = true>
    constexpr auto transform(F&& f) const && {
        return transform_impl(::std::move(*this), ::std::forward<F>(f));
    }
    
    template<typename F, _internal::expected_monad_constraint_t<F, T, ::std::add_lvalue_reference_t<T>> = true>
    constexpr auto transform_error(F&& f) & {
        return transform_error_impl(*this, ::std::forward<F>(f));
    }
    template<typename F, _internal::expected_monad_constraint_t<F, T, ::std::add_lvalue_reference_t<const T>> = true>
    constexpr auto transform_error(F&& f) const & {
        return transform_error_impl(*this, ::std::forward<F>(f));
    }

    template<typename F, _internal::expected_monad_constraint_t<F, T, ::std::add_rvalue_reference_t<T>> = true>
    constexpr auto transform_error(F&& f) && {
        return transform_error_impl(::std::move(*this), ::std::forward<F>(f));
    }
    template<typename F, _internal::expected_monad_constraint_t<F, T, ::std::add_rvalue_reference_t<const T>> = true>
    constexpr auto transform_error(F&& f) const && {
        return transform_error_impl(::std::move(*this), ::std::forward<F>(f));
    }
    
    // === comparators
    template<
        typename T2, typename E2,
        ::std::enable_if_t<
            ((::std::is_void<T>::value && ::std::is_void<T2>::value) || _internal::expected_eq_test<::std::add_lvalue_reference_t<const T>, const T2>::value) &&
            _internal::expected_eq_test<const E&, const E2&>::value,
        bool> = true
    >
    friend constexpr bool operator==(const expected<T, E>& x, const expected<T2, E2>& y) {
        if (x.has_value() != y.has_value())
            return false;
        return x.has_value() ? compare_expected_values(::std::is_void<T>{}, x, y) : static_cast<bool>(x.error() == y.error());
    }

    template<typename E2, ::std::enable_if_t<_internal::expected_eq_test<const E&, const E2&>::value, bool> = true>
    friend constexpr bool operator==(const expected<T, E>& x, const unexpected<E2>& e) {
        return !x.has_value() && static_cast<bool>(x.error() == e.error());
    }
    
#if __cpp_impl_three_way_comparison < 201907L
    // suppliment for synthesized ops
    template<
        typename T2, typename E2,
        ::std::enable_if_t<
            ((::std::is_void<T>::value && ::std::is_void<T2>::value) || _internal::expected_eq_test<::std::add_lvalue_reference_t<const T>, const T2>::value) &&
            _internal::expected_eq_test<const E&, const E2&>::value,
        bool> = true
    >
    friend constexpr bool operator!=(const expected<T, E>& x, const expected<T2, E2>& y) {
        return !(x == y);
    }

    template<typename E2, ::std::enable_if_t<_internal::expected_eq_test<const E&, const E2&>::value, bool> = true>
    friend constexpr bool operator!=(const expected<T, E>& x, const unexpected<E2>& e) {
        return !(x == e);
    }
    template<typename E2, ::std::enable_if_t<_internal::expected_eq_test<const E&, const E2&>::value, bool> = true>
    friend constexpr bool operator!=(const unexpected<E2>& e, const expected<T, E>& x) {
        return x != e;
    }
    template<typename E2, ::std::enable_if_t<_internal::expected_eq_test<const E&, const E2&>::value, bool> = true>
    friend constexpr bool operator==(const unexpected<E2>& e, const expected<T, E>& x) {
        return x == e;
    }
#endif

protected:
    constexpr expected_toplevel_base(dummy_t) : swappable_base<T, E>{dummy_t{}} {}

private:
    template<typename... Us>
    constexpr void forward_construct_e(Us&&... us) {
        ::new (::std::addressof(this->_unex)) E(::std::forward<Us>(us)...);
        this->_has_val = false;
    }

    template<typename Expected_T>
    constexpr void forward_construct_compat_expected_value(::std::true_type, Expected_T&&) {
        ::new (::std::addressof(this->_val)) expected_value_member_t<T>();
    }
    template<typename Expected_T>
    constexpr void forward_construct_compat_expected_value(::std::false_type, Expected_T&& other) {
        ::new (::std::addressof(this->_val)) expected_value_member_t<T>(*::std::forward<Expected_T>(other));
    }

    template<typename Expected_T>
    constexpr void forward_construct_compat_expected(Expected_T&& other) {
        if (other.has_value())
            forward_construct_compat_expected_value(::std::is_void<T>{}, ::std::forward<Expected_T>(other));
        else
            ::new (::std::addressof(this->_unex)) E(::std::forward<Expected_T>(other).error());
        this->_has_val = other.has_value();
    }

    template<typename U, typename This_T, typename F>
    static constexpr auto transform_impl(::std::true_type, This_T&& this_ref, F&& f) {
        make_monad_invoke_ret(::std::is_void<T>{}, ::std::forward<F>(f), ::std::forward<This_T>(this_ref)._val);
        return expected<U, E>();
    }
    template<typename U, typename This_T, typename F>
    static constexpr auto transform_impl(::std::false_type, This_T&& this_ref, F&& f) {
        return expected<U, E>(in_place, make_monad_invoke_ret(::std::is_void<T>{}, ::std::forward<F>(f), ::std::forward<This_T>(this_ref)._val));
    }
    template<typename This_T, typename F>
    static constexpr auto transform_impl(This_T&& this_ref, F&& f) {
        using U = expected_invoke_result_t<F, decltype(::std::forward<This_T>(this_ref)._val)>;
    
        if (!this_ref._has_val)
            return expected<U, E>(unexpect, ::std::forward<This_T>(this_ref).error());
        return transform_impl<U>(::std::integral_constant<bool, ::std::is_void<U>::value>{}, ::std::forward<This_T>(this_ref), ::std::forward<F>(f));
    }
    
    template<typename This_T, typename F>
    static constexpr auto transform_error_impl(This_T&& this_ref, F&& f) {
        using G = ::std::remove_cv_t<invoke_result_t<F, decltype(::std::forward<This_T>(this_ref).error())>>;
    
        if (this_ref._has_val)
            return make_monad_value_ret<expected<T, G>>(::std::is_void<T>{}, ::std::forward<This_T>(this_ref)._val);
        return expected<T, G>(unexpect, invoke(::std::forward<F>(f), ::std::forward<This_T>(this_ref).error()));
    }
};

} // <<< internal

// === expected
template<typename T, typename E>
class expected : public _internal::expected_toplevel_base<T, E> {
    // compat T ctor traits
    template<typename U>
    static constexpr bool compat_t_explicit_v() {
        return ::std::is_convertible<U, T>::value;
    }

    // msvc 19.16.27050.0 refuses to treat constexpr vars or functions as const evaluated in sfinae contexts, macros instead
#define IDYM_COMPAT_T_CONSTRAINT_V \
    (!::std::is_same<remove_cvref_t<U>, in_place_t>::value && \
    !::std::is_same<remove_cvref_t<U>, expected>::value && \
    !::std::is_same<remove_cvref_t<U>, unexpect_t>::value && \
    !_internal::is_specialization_of_v<remove_cvref_t<U>, ::IDYM_NAMESPACE::unexpected> && \
    ::std::is_constructible<T, U>::value && \
    (!::std::is_same<bool, ::std::remove_cv_t<T>>::value || !_internal::is_specialization_of_v<remove_cvref_t<U>, ::IDYM_NAMESPACE::expected>))

#define IDYM_UNEXPECTED_ASS_CONSTRAINT_V \
    (::std::is_constructible<E, GF>::value && \
    ::std::is_assignable<E&, GF>::value && \
    (::std::is_nothrow_constructible<E, GF>::value || ::std::is_nothrow_move_constructible<T>::value || ::std::is_nothrow_move_constructible<E>::value))

public:
    // === ctors
    constexpr expected() = default;

    using _internal::expected_toplevel_base<T, E>::expected_toplevel_base;
    
    template<
        typename U = ::std::remove_cv_t<T>,
        ::std::enable_if_t<::std::is_convertible<U, T>::value && IDYM_COMPAT_T_CONSTRAINT_V, bool> = true
    >
    constexpr explicit expected(U&& v) : expected{_internal::dummy_t{}} {
        forward_construct_t(std::forward<U>(v));
    }
    template<
        typename U = ::std::remove_cv_t<T>,
        ::std::enable_if_t<!::std::is_convertible<U, T>::value && IDYM_COMPAT_T_CONSTRAINT_V, bool> = true
    >
    constexpr expected(U&& v) : expected{_internal::dummy_t{}} {
        forward_construct_t(std::forward<U>(v));
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

    // === assignment
    template<
        typename U = ::std::remove_cv_t<T>,
        ::std::enable_if_t<
            !::std::is_same<expected, remove_cvref_t<U>>::value &&
            !_internal::is_specialization_of_v<remove_cvref_t<U>, ::IDYM_NAMESPACE::unexpected> &&
            ::std::is_constructible<T, U>::value &&
            ::std::is_assignable<T&, U>::value &&
            (::std::is_nothrow_constructible<T, U>::value || ::std::is_nothrow_move_constructible<T>::value || ::std::is_nothrow_move_constructible<E>::value),
        bool> = true
    >
    constexpr expected& operator=(U&& v) {
        if (this->_has_val) {
            this->_val = ::std::forward<U>(v);
        } else {
            _internal::reinit_expected(this->_val, this->_unex, ::std::forward<U>(v));
            this->_has_val = true;
        }
        return *this;
    }

    template<typename G, typename GF = const G&, ::std::enable_if_t<IDYM_UNEXPECTED_ASS_CONSTRAINT_V, bool> = true>
    constexpr expected& operator=(const unexpected<G>& e) {
        return forward_assign_e(e.error());
    }

    template<typename G, typename GF = G, ::std::enable_if_t<IDYM_UNEXPECTED_ASS_CONSTRAINT_V, bool> = true>
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

    constexpr const T& value() const & {
        if (this->_has_val)
            return this->_val;
        throw bad_expected_access<E>(as_const(this->_unex));
    }
    constexpr T& value() & {
        if (this->_has_val)
            return this->_val;
        throw bad_expected_access<E>(as_const(this->_unex));
    }

    constexpr T&& value() && {
        if (this->_has_val)
            return ::std::move(this->_val);
        throw bad_expected_access<E>(::std::move(this->_unex));
    }
    constexpr const T&& value() const && {
        if (this->_has_val)
            return ::std::move(this->_val);
        throw bad_expected_access<E>(::std::move(this->_unex));
    }

    template<typename U = remove_cvref_t<T>>
    constexpr T value_or(U&& v) const & {
        return this->_has_val ? **this : static_cast<T>(::std::forward<U>(v));
    }
    template<typename U = remove_cvref_t<T>>
    constexpr T value_or(U&& v)  && {
        return this->_has_val ? ::std::move(**this) : static_cast<T>(::std::forward<U>(v));
    }
    
    // === comparators
    template<
        typename T2,
        ::std::enable_if_t<!_internal::is_specialization_of_v<T2, ::IDYM_NAMESPACE::expected> && _internal::expected_eq_test<const T&, const T2&>::value, bool> = true
    >
    friend constexpr bool operator==(const expected& x, const T2& v) {
        return x.has_value() && static_cast<bool>(*x == v);
    }

#if __cpp_impl_three_way_comparison < 201907L
    // suppliment for synthesized ops
    template<
        typename T2,
        ::std::enable_if_t<!_internal::is_specialization_of_v<T2, ::IDYM_NAMESPACE::expected> && _internal::expected_eq_test<const T&, const T2&>::value, bool> = true
    >
    friend constexpr bool operator!=(const expected& x, const T2& v) {
        return !(x == v);
    }
    template<
        typename T2,
        ::std::enable_if_t<!_internal::is_specialization_of_v<T2, ::IDYM_NAMESPACE::expected> && _internal::expected_eq_test<const T&, const T2&>::value, bool> = true
        >
    friend constexpr bool operator!=(const T2& v, const expected& x) {
        return x != v;
    }
    template<
        typename T2,
        ::std::enable_if_t<!_internal::is_specialization_of_v<T2, ::IDYM_NAMESPACE::expected> && _internal::expected_eq_test<const T&, const T2&>::value, bool> = true
    >
    friend constexpr bool operator==(const T2& v, const expected& x) {
        return x == v;
    }
#endif

private:
    constexpr expected(_internal::dummy_t) : _internal::expected_toplevel_base<T, E>{_internal::dummy_t{}} {}

    template<typename... Us>
    constexpr void forward_construct_t(Us&&... us) {
        ::new (::std::addressof(this->_val)) T(::std::forward<Us>(us)...);
        this->_has_val = true;
    }

    template<typename G>
    constexpr expected& forward_assign_e(G&& g) {
        if (this->_has_val) {
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

namespace _internal { // >>> internal

template<typename T, typename E>
struct expected_void_toplevel_base : expected_toplevel_base<T, E> {
    using expected_toplevel_base<T, E>::expected_toplevel_base;
    
    constexpr void emplace() noexcept {
        if (!this->_has_val) {
            this->_unex.~E();
            this->_has_val = true;
        }
    }
    
    constexpr void operator*() const noexcept {}

    constexpr void value() const & {
        if (!this->_has_val)
            throw bad_expected_access<E>(this->_unex);
    }
    constexpr void value() && {
        if (!this->_has_val)
            throw bad_expected_access<E>(::std::move(this->_unex));
    }
};

} // <<< internal

// === expected<cv void, E>
template<typename E>
class expected<void, E> : public _internal::expected_void_toplevel_base<void, E> {
public:
    using _internal::expected_void_toplevel_base<void, E>::expected_void_toplevel_base;
};
template<typename E>
class expected<const void, E> : public _internal::expected_void_toplevel_base<const void, E> {
public:
    using _internal::expected_void_toplevel_base<const void, E>::expected_void_toplevel_base;
};
template<typename E>
class expected<volatile void, E> : public _internal::expected_void_toplevel_base<volatile void, E> {
public:
    using _internal::expected_void_toplevel_base<volatile void, E>::expected_void_toplevel_base;
};
template<typename E>
class expected<const volatile void, E> : public _internal::expected_void_toplevel_base<const volatile void, E> {
public:
    using _internal::expected_void_toplevel_base<const volatile void, E>::expected_void_toplevel_base;
};

}

#undef IDYM_COMPAT_EXPECTED_EXPLICIT_V
#undef IDYM_COMPAT_EXPECTED_CONSTRAINT_V
#undef IDYM_COMPAT_T_CONSTRAINT_V
#undef IDYM_COMPAT_T_CONSTRAINT_V

#endif
