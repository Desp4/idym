#ifndef IDYM_VARIANT_H
#define IDYM_VARIANT_H

#include <utility>
#include <cstddef>
#include <initializer_list>

#include "type_traits.hpp"
#include "utility.hpp"

namespace idym {

template<typename...>
class variant;

// === variant_npos
constexpr ::std::size_t variant_npos = -1; // TODO: c++17 inline

namespace _internal { // >>> internal

// === first_of
template<typename T, typename... Ts>
struct first_of {
    using type = T;
};
template<typename... Ts>
using first_of_t = typename first_of<Ts...>::type;

// === variant_storage
template<bool, typename... Ts>
union variant_storage_impl;

template<typename... Ts>
using variant_storage = variant_storage_impl<conjunction_v<::std::is_trivially_destructible<Ts>...>, Ts...>;

template<bool Trivial_Dtor> union variant_storage_impl<Trivial_Dtor> {};

template<typename T, typename... Ts>
union variant_storage_impl<true, T, Ts...> {
    T v0;
    variant_storage<Ts...> v1;
    
    constexpr variant_storage_impl() noexcept : v1{} {}
};

template<typename T, typename... Ts>
union variant_storage_impl<false, T, Ts...> {
    T v0;
    variant_storage<Ts...> v1;
    
    constexpr variant_storage_impl() noexcept : v1{} {}
    ~variant_storage_impl() {} // TODO: c++20, constexpr
};

// === alternative destruction
template<::std::size_t I, typename Storage_T>
constexpr void destroy_alternative(::std::size_t index, Storage_T& variant) {
    using v0_t = decltype(variant.v0);
    if (index == I)
        variant.v0.~v0_t();
    else
        destroy_alternative<I + 1>(index, variant.v1);
}
template<::std::size_t>
constexpr void destroy_alternative(::std::size_t, variant_storage<>&) {}

// === apply generic functor to alternatives
template<::std::size_t I, typename Fun, typename Storage_T>
constexpr void apply_to_alternatives(::std::size_t index, Fun apply_fun, Storage_T& lhs, Storage_T& rhs) {
    if (index == I)
        apply_fun(&lhs.v0, &rhs.v0);
    else
        apply_to_alternatives<I + 1>(index, apply_fun, lhs.v1, rhs.v1);
}

template<::std::size_t, typename Fun>
constexpr void apply_to_alternatives(::std::size_t, Fun, variant_storage<>&, variant_storage<>&) {}

// === generic constructor callables, to be applied with the function above
struct copy_construct_alternative {
    template<typename Container_T>
    constexpr void operator()(Container_T* lhs, const Container_T* rhs) const {
        ::new (lhs) Container_T{*rhs};
    }
};
struct move_construct_alternative {
    template<typename Container_T>
    constexpr void operator()(Container_T* lhs, Container_T* rhs) const {
        ::new (lhs) Container_T{::std::move(*rhs)};
    }
};
struct copy_assign_alternative {
    template<typename Container_T>
    constexpr void operator()(Container_T* lhs, const Container_T* rhs) const {
        *lhs = *rhs;
    }
};
struct move_assign_alternative {
    template<typename Container_T>
    constexpr void operator()(Container_T* lhs, Container_T* rhs) const {
        *lhs = ::std::move(*rhs);
    }
};
struct swap_alternative {
    template<typename Container_T>
    constexpr void operator()(Container_T* lhs, Container_T* rhs) const {
        using ::std::swap;
        swap(*lhs, *rhs);
    }
};

// === generic copy/move of two variants
template<typename Var_Lhs, typename Var_Rhs, typename Ctor_Fun, typename Ass_Fun>
constexpr auto& assign_variants(Var_Lhs&& lhs, Var_Rhs&& rhs, Ctor_Fun ctor, Ass_Fun ass) {
    if (rhs._index == variant_npos) {
        if (lhs._index != variant_npos) {
            destroy_alternative<0>(lhs._index, lhs._storage);
            lhs._index = variant_npos;
        }
        return lhs;
    }
    
    if (lhs._index == rhs._index) {
        apply_to_alternatives<0>(lhs._index, ass, lhs._storage, rhs._storage);
    } else {
        if (lhs._index != variant_npos) {
            destroy_alternative<0>(lhs._index, lhs._storage);
            lhs._index = variant_npos;
        }
        
        apply_to_alternatives<0>(rhs._index, ctor, lhs._storage, rhs._storage);
        lhs._index = rhs._index;
    }
    return lhs;
}

// === internal get
template<::std::size_t I>
struct get_variant_storage {
    template<typename Storage_T>
    constexpr static auto do_get(Storage_T&& storage) {
        return get_variant_storage<I - 1>(storage.v1);
    }
};
template<>
struct get_variant_storage<0> {
    template<typename Storage_T>
    constexpr static auto do_get(Storage_T&& storage) {
        return &storage.v0;
    }
};

// === variant_base
template<typename... Ts>
struct variant_base {
    variant_storage<Ts...> _storage;
    ::std::size_t _index = variant_npos;
};

/*
 * For each base, the first bool is:
 * - true for trivial OR deleted
 * - false otherwise(non-trivial)
 */
// === variant_base_dtor
template<bool, typename...>
struct variant_base_dtor;

template<typename... Ts>
struct variant_base_dtor<true, Ts...> : variant_base<Ts...> {};

template<typename... Ts>
struct variant_base_dtor<false, Ts...> : variant_base_dtor<true, Ts...> {
    ~variant_base_dtor() { // TODO: c++20 constexpr
        if (this->_index != variant_npos)
            destroy_alternative<0>(this->_index, this->_storage);
    }
};

// === variant_base_copy_ctor
template<bool, typename...>
struct variant_base_copy_ctor;

template<typename... Ts>
struct variant_base_copy_ctor<true, Ts...> : variant_base_dtor<
    conjunction_v<::std::is_trivially_destructible<Ts>...>,
    Ts...
> {};

template<typename... Ts>
struct variant_base_copy_ctor<false, Ts...> : variant_base_copy_ctor<true, Ts...> {
    constexpr variant_base_copy_ctor(const variant_base_copy_ctor& other) {
        if (other._index == variant_npos)
            return;
        apply_to_alternatives<0>(other._index, copy_construct_alternative{}, this->_storage, other._storage);
        this->_index = other._index;
    }
};

// === variant_base_move_ctor
template<bool, typename...>
struct variant_base_move_ctor;

template<typename... Ts>
struct variant_base_move_ctor<true, Ts...> : variant_base_copy_ctor<
    conjunction_v<::std::is_trivially_copy_constructible<Ts>...>,
    Ts...
>
{
    constexpr variant_base_move_ctor(variant_base_move_ctor&&) noexcept(conjunction_v<::std::is_nothrow_move_constructible<Ts>...>) = default;
};

template<typename... Ts>
struct variant_base_move_ctor<false, Ts...> : variant_base_move_ctor<true, Ts...> {
    constexpr variant_base_move_ctor(variant_base_move_ctor&& other) noexcept(conjunction_v<::std::is_nothrow_move_constructible<Ts>...>) {
        if (other._index == variant_npos)
            return;
        apply_to_alternatives<0>(other._index, move_construct_alternative{}, this->_storage, other._storage);
        this->_index = other._index;
    }
};

// === variant_base_copy_ass
template<bool, typename...>
struct variant_base_copy_ass;

template<typename... Ts>
struct variant_base_copy_ass<true, Ts...> : variant_base_move_ctor<
    conjunction_v<::std::is_trivially_move_constructible<Ts>...> || !conjunction_v<::std::is_move_constructible<Ts>...>,
    Ts...
> {};

template<typename... Ts>
struct variant_base_copy_ass<false, Ts...> : variant_base_move_ctor<true, Ts...> {
    constexpr variant_base_copy_ass& operator=(const variant_base_copy_ass& other) {
        return assign_variants(*this, other, copy_construct_alternative{}, copy_assign_alternative{});
    }
};

// === variant_base_move_ass
template<bool, typename...>
struct variant_base_move_ass;

template<typename... Ts>
struct variant_base_move_ass<true, Ts...> : variant_base_copy_ass<
    (
        conjunction_v<::std::is_trivially_copy_constructible<Ts>...> && 
        conjunction_v<::std::is_trivially_copy_assignable<Ts>...> && 
        conjunction_v<::std::is_trivially_destructible<Ts>...>
    ) || (!conjunction_v<::std::is_copy_constructible<Ts>...> || !conjunction_v<::std::is_copy_assignable<Ts>...>),
    Ts...
>
{
    static constexpr auto ass_nothrow = conjunction_v<::std::is_nothrow_move_constructible<Ts>...> && conjunction_v<::std::is_nothrow_move_assignable<Ts>...>;
    constexpr variant_base_move_ass& operator=(variant_base_move_ass&&) noexcept(ass_nothrow) = default;
};

template<typename... Ts>
struct variant_base_move_ass<false, Ts...> : variant_base_move_ass<true, Ts...> {
    constexpr variant_base_move_ass& operator=(variant_base_move_ass&& other) noexcept(variant_base_move_ass<true, Ts...>::ass_nothrow) {
        return assign_variants(*this, other, move_construct_alternative{}, move_assign_alternative{});
    }
};

// === variant_base_final
template<typename... Ts>
using variant_base_final = variant_base_move_ass<
    (
        conjunction_v<::std::is_trivially_move_constructible<Ts>...> &&
        conjunction_v<::std::is_trivially_move_assignable<Ts>...> &&
        conjunction_v<::std::is_trivially_destructible<Ts>...>
    ) || (!conjunction_v<::std::is_move_constructible<Ts>...> || !conjunction_v<::std::is_move_assignable<Ts>...>),
    Ts...
>;

// === constraint checks
template<typename T>
struct instanceof_in_place_type : ::std::false_type {};
template<typename T>
struct instanceof_in_place_type<in_place_type_t<T>> : ::std::true_type {};

template<typename T>
struct instanceof_in_place_index : ::std::false_type {};
template<::std::size_t I>
struct instanceof_in_place_index<in_place_index_t<I>> : ::std::true_type {};

// === varaint(T&&) and operator=(T&&) constraint deduction
template<typename T>
struct variant_overload {
    T operator()(T (&&)[1]);
};

template<typename...>
struct variant_overload_set;

template<typename T, typename... Ts>
struct variant_overload_set<T, Ts...> : variant_overload<T>, variant_overload_set<Ts...> {
    using variant_overload<T>::operator();
    using variant_overload_set<Ts...>::operator();
};

template<> struct variant_overload_set<> {};

template<typename Arg_T, typename... Ts>
using variant_ctor_compat_t = decltype(::std::declval<variant_overload_set<Ts...>>()({::std::declval<Arg_T>()}));

// === alternative_to_index
template<::std::size_t, typename, typename...>
struct alternative_to_index : ::std::integral_constant<::std::size_t, variant_npos> {};

template<::std::size_t I, typename Target_T, typename T, typename... Ts>
struct alternative_to_index<I, Target_T, T, Ts...> : alternative_to_index<I + 1, Target_T, Ts...> {};

template<::std::size_t I, typename Target_T, typename... Ts>
struct alternative_to_index<I, Target_T, Target_T, Ts...> : ::std::integral_constant<::std::size_t, I> {};

// === index_to_alternative
template<::std::size_t, typename... Ts>
struct index_to_alternative;

template<::std::size_t I>
struct index_to_alternative<I> {};
template<::std::size_t I, typename T, typename... Ts>
struct index_to_alternative<I, T, Ts...> : index_to_alternative<I - 1, Ts...> {};

template<typename T, typename... Ts>
struct index_to_alternative<0, T, Ts...> {
    using type = T;
};

template<::std::size_t I, typename... Ts>
using index_to_alternative_t = typename index_to_alternative<I, Ts...>::type;

// type_occurrence_count
template<typename, typename...>
struct type_occurrence_count;

template<typename Target_T, typename T, typename... Ts>
struct type_occurrence_count<Target_T, T, Ts...> {
    static constexpr ::std::size_t value = type_occurrence_count<Target_T, Ts...>::value;
};
template<typename Target_T, typename... Ts>
struct type_occurrence_count<Target_T, Target_T, Ts...> {
    static constexpr ::std::size_t value = 1 + type_occurrence_count<Target_T, Ts...>::value;
};
template<typename Target_T>
struct type_occurrence_count<Target_T> {
    static constexpr ::std::size_t value = 0;
};

// alternative constructor
template<::std::size_t I, typename... Alt_Ts, typename... Ts>
constexpr auto* init_alternative_at(variant_base<Alt_Ts...>&& storage, Ts&&... args) {
    auto* alt_ptr = get_variant_storage<I>::do_get(storage._storage);
    ::new (alt_ptr) ::std::remove_pointer_t<decltype(alt_ptr)>(std::forward<Ts>(args)...);
    storage._index = I;
    return alt_ptr;
}

} // <<< internal


// === variant_size
template<typename>
struct variant_size;

template<typename... Ts>
struct variant_size<variant<Ts...>> : ::std::integral_constant<::std::size_t, sizeof...(Ts)> {};
template<typename T>
struct variant_size<const T> : variant_size<T> {};
template<typename T>
struct variant_size<volatile T> : variant_size<T> {}; // TODO: c++20 deprecated
template<typename T>
struct variant_size<const volatile T> : variant_size<T> {}; // TODO: c++20 deprecated

template<typename T>
constexpr ::std::size_t variant_size_v = variant_size<T>::value;

// === variant_alternative
template<::std::size_t I, typename T>
struct variant_alternative;

template<::std::size_t I, typename... Ts>
struct variant_alternative<I, variant<Ts...>> : _internal::index_to_alternative<I, Ts...> {};
template<::std::size_t I, typename T>
struct variant_alternative<I, const T> : variant_alternative<I, T> {};
template<::std::size_t I, typename T>
struct variant_alternative<I, volatile T> : variant_alternative<I, T> {}; // TODO: c++20 deprecated
template<::std::size_t I, typename T>
struct variant_alternative<I, const volatile T> : variant_alternative<I, T> {}; // TODO: c++20 deprecated

template<::std::size_t I, typename T>
using variant_alternative_t = typename variant_alternative<I, T>::type;

// === variant
template<typename... Ts>
class variant : _internal::variant_base_final<Ts...> {
    static_assert(sizeof...(Ts) > 0, "variant must have at least one alternative");
    
public:
    template<::std::enable_if_t<::std::is_default_constructible<_internal::first_of_t<Ts...>>::value, bool> = true>
    constexpr variant() noexcept(::std::is_nothrow_default_constructible<_internal::first_of_t<Ts...>>::value) {
        ::new (&this->_storage.v0) _internal::first_of_t<Ts...>{};
        this->_index = 0;
    }
    
    template<
        typename T,
        ::std::enable_if_t<
            !::std::is_same<remove_cvref_t<T>, variant>::value &&
            !_internal::instanceof_in_place_type<remove_cvref_t<T>>::value &&
            !_internal::instanceof_in_place_index<remove_cvref_t<T>>::value,
        bool> = true,
        ::std::enable_if_t<
            ::std::is_constructible<_internal::variant_ctor_compat_t<T, Ts...>, T>::value,
        bool> = true
    >
    constexpr variant(T&& t) noexcept(::std::is_nothrow_constructible<_internal::variant_ctor_compat_t<T, Ts...>, T>::value) :
        variant{in_place_type<_internal::variant_ctor_compat_t<T, Ts...>>, ::std::forward<T>(t)}
    {
    }
    
    template<
        typename T, typename... Args,
        ::std::enable_if_t<
            _internal::type_occurrence_count<T, Ts...>::value == 1 &&
            ::std::is_constructible<T, Args...>::value,
        bool> = true
    >
    constexpr explicit variant(in_place_type_t<T>, Args&&... args) {
        _internal::init_alternative_at<_internal::alternative_to_index<0, T, Ts...>::value>(*this, std::forward<Args>(args)...);
    }

    template<
        typename T, typename U, typename... Args,
        ::std::enable_if_t<
            _internal::type_occurrence_count<T, Ts...>::value == 1 &&
            ::std::is_constructible<T, ::std::initializer_list<U>&, Args...>::value,
        bool> = true
    >
    constexpr explicit variant(in_place_type_t<T>, ::std::initializer_list<U> il, Args&&... args) {
        _internal::init_alternative_at<_internal::alternative_to_index<0, T, Ts...>::value>(*this, il, std::forward<Args>(args)...);
    }
    
    template<
        ::std::size_t I, typename... Args,
        ::std::enable_if_t<
            I < sizeof...(Ts) &&
            ::std::is_constructible<_internal::index_to_alternative_t<I, Ts...>, Args...>::value,
        bool> = true
    >
    constexpr explicit variant(in_place_index_t<I>, Args&&... args) {
        _internal::init_alternative_at<I>(*this, std::forward<Args>(args)...);
    }
    
    template<
        ::std::size_t I, typename U, typename... Args,
        ::std::enable_if_t<
            I < sizeof...(Ts) &&
            ::std::is_constructible<_internal::index_to_alternative_t<I, Ts...>, ::std::initializer_list<U>&, Args...>::value,
        bool> = true
    >
    constexpr explicit variant(in_place_index_t<I>, ::std::initializer_list<U> il, Args&&... args) {
        _internal::init_alternative_at<I>(*this, il, std::forward<Args>(args)...);
    }
    
    template<
        typename T,
        ::std::enable_if_t<
            !::std::is_same<remove_cvref_t<T>, variant>::value &&
            ::std::is_assignable<_internal::variant_ctor_compat_t<T, Ts...>&, T>::value &&
            ::std::is_constructible<_internal::variant_ctor_compat_t<T, Ts...>, T>::value,
        bool> = true
    >
    constexpr variant& operator=(T&& t) noexcept(
        ::std::is_nothrow_assignable<_internal::variant_ctor_compat_t<T, Ts...>&, T>::value &&
        ::std::is_nothrow_constructible<_internal::variant_ctor_compat_t<T, Ts...>, T>::value
    )
    {
        using alt_t = _internal::variant_ctor_compat_t<T, Ts...>;
        constexpr auto alt_ind = _internal::alternative_to_index<0, alt_t, Ts...>::value;
        constexpr auto direct_emplace = ::std::is_nothrow_constructible<alt_t, T>::value || !::std::is_nothrow_move_constructible<alt_t>::value;
        
        if (this->_index == alt_ind)
            *_internal::get_variant_storage<alt_ind>::do_get(this->_storage) = std::forward<T>(t);
        else
            assign_impl<alt_ind>(::std::bool_constant<direct_emplace>{}, ::std::forward<T>(t));
        return *this;
    }
    
    template<
        typename T,
        typename... Args,
        ::std::enable_if_t<
            _internal::type_occurrence_count<T, Ts...>::value == 1 &&
            ::std::is_constructible<T, Args...>::value,
        bool> = true
    >
    constexpr T& emplace(Args&&... args) {
        return emplace<_internal::alternative_to_index<0, T, Ts...>::value>(std::forward<Args>(args)...);
    }
    
    template<
        typename T,
        typename U,
        typename... Args,
        ::std::enable_if_t<
            _internal::type_occurrence_count<T, Ts...>::value == 1 &&
            ::std::is_constructible<T, ::std::initializer_list<U>&, Args...>::value,
        bool> = true
    >
    constexpr T& emplace(::std::initializer_list<U> il, Args&&... args) {
        return emplace<_internal::alternative_to_index<0, T, Ts...>::value>(il, std::forward<Args>(args)...);
    }
    
    template<
        ::std::size_t I,
        typename... Args,
        ::std::enable_if_t<
            ::std::is_constructible<_internal::index_to_alternative_t<I, Ts...>, Args...>::value,
        bool> = true
    >
    constexpr variant_alternative<I, variant<Ts...>>& emplace(Args&&... args) {
        if (this->_index != variant_npos) {
            _internal::destroy_alternative<0>(this->_index, this->_storage);
            this->_index = variant_npos;
        }
        return *_internal::init_alternative_at<I>(*this, std::forward<Args>(args)...);
    }
    
    template<
        ::std::size_t I,
        typename U,
        typename... Args,
        ::std::enable_if_t<
            ::std::is_constructible<_internal::index_to_alternative_t<I, Ts...>, ::std::initializer_list<U>&, Args...>::value,
        bool> = true
    >
    constexpr variant_alternative<I, variant<Ts...>>& emplace(::std::initializer_list<U> il, Args&&... args) {
        if (this->_index != variant_npos) {
            _internal::destroy_alternative<0>(this->_index, this->_storage);
            this->_index = variant_npos;
        }
        return *_internal::init_alternative_at<I>(*this, il, ::std::forward<Args>(args)...);
    }
    
    constexpr bool valueless_by_exception() const noexcept {
        return this->_index == variant_npos;
    }
    constexpr ::std::size_t index() const noexcept {
        return this->_index;
    }
    
    constexpr void swap(variant& rhs) noexcept(conjunction_v<::std::is_nothrow_move_constructible<Ts>...> && conjunction_v<is_nothrow_swappable<Ts>...>) {
        if (this->_index == variant_npos) {
            if (rhs._index == variant_npos)
                return;
            
            _internal::apply_to_alternatives<0>(rhs._index, _internal::move_construct_alternative{}, this->_storage, rhs._storage);
            ::std::swap(this->_index, rhs._index);
            return;
        }

        if (rhs._index == variant_npos) {
            _internal::apply_to_alternatives<0>(this->_index, _internal::move_construct_alternative{}, rhs._storage, this->_storage);
            return;
        }

        if (this->_index == rhs._index) {
            _internal::apply_to_alternatives<0>(this->_index, _internal::swap_alternative{}, this->_storage, rhs._storage);
            return;
        }
        
        const auto old_lhs_ind = this->_index;
        const auto old_rhs_ind = rhs._index;
        
        variant tmp_rhs{std::move(rhs)};
        _internal::apply_to_alternatives<0>(old_lhs_ind, _internal::move_construct_alternative{}, rhs._storage, this->_storage);
        rhs._storage = old_lhs_ind;
        
        _internal::apply_to_alternatives<0>(old_rhs_ind, _internal::move_construct_alternative{}, this->_storage, tmp_rhs._storage);
        this->_index = old_rhs_ind;
    }
    
private:
    template<::std::size_t I, typename T>
    constexpr void assign_impl(::std::true_type, T&& t) {
        emplace<I>(::std::forward<T>(t));
    }
    template<::std::size_t I, typename T>
    constexpr void assign_impl(::std::false_type, T&& t) {
        emplace<I>(_internal::index_to_alternative_t<I>(::std::forward<T>(t)));
    }
};

}

#endif
