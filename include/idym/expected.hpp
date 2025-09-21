#ifndef IDYM_EXPECTED_H
#define IDYM_EXPECTED_H

#include <exception>
#include <initializer_list>

#include "utility.hpp"
#include "type_traits.hpp"

namespace IDYM_NAMESPACE {

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

namespace _internal { // >>> internal

// === expected_base
template<bool, typename, typename>
struct expected_base_impl;

template<typename T, typename E>
struct expected_base_impl<true, T, E> {
    union {
        T value;
        E unex;
        union {} dummy;
    };
    bool has_val;
    
    constexpr expected_base_impl() noexcept : dummy{} {}
};
template<typename T, typename E>
struct expected_base_impl<false, T, E> {
    union {
        T value;
        E unex;
        union {} dummy;
    };
    bool has_val;

    constexpr expected_base_impl() noexcept : dummy{} {}
    IDYM_INTERNAL_CXX20_CONSTEXPR_DTOR ~expected_base_impl() {
        if (has_val)
            value.~T();
        else
            unex.~E();
    }
};

template<typename T, typename E>
using expected_base = expected_base_impl<::std::is_trivially_destructible<T>::value && ::std::is_trivially_destructible<E>::value, T, E>;

// === expected_def_ctor_base
template<bool, typename, typename>
struct expected_def_ctor_base_impl;

template<typename T, typename E>
struct expected_def_ctor_base_impl<true, T, E> : expected_base<T, E> {
    constexpr expected_def_ctor_base_impl() {
        ::new (&this->value) T();
        this->has_val = true;
    }
    constexpr expected_def_ctor_base_impl(dummy_t) {}
};
template<typename T, typename E>
struct expected_def_ctor_base_impl<false, T, E> : expected_base<T, E> {
    expected_def_ctor_base_impl() = delete;
    constexpr expected_def_ctor_base_impl(dummy_t) {}
};

template<typename T, typename E>
struct expected_def_ctor_base : expected_def_ctor_base_impl<::std::is_default_constructible<T>::value, T, E> {
    constexpr expected_def_ctor_base() = default;
    constexpr expected_def_ctor_base(const expected_def_ctor_base&) = default;
    constexpr expected_def_ctor_base(expected_def_ctor_base&&) noexcept(::std::is_nothrow_move_constructible<T>::value && ::std::is_nothrow_move_constructible<E>::value) = default;
};

} // <<< internal

// === expected
template<typename T, typename E>
class expected {
public:

private:
};

}

#endif
