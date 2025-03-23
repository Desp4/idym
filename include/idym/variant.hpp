#ifndef IDYM_VARIANT_H
#define IDYM_VARIANT_H

#include <array>
#include <utility>
#include <cstddef>
#include <exception>
#include <functional>
#include <initializer_list>

#include "type_traits.hpp"
#include "utility.hpp"

namespace std {
template<typename> struct hash;
}

namespace idym {

template<typename...>
class variant;

// === variant_npos
constexpr ::std::size_t variant_npos = -1; // TODO: c++17 inline

// === monostate
struct monostate{};

inline constexpr bool operator==(monostate, monostate) noexcept { return true; }
// TODO: c++20 spaceship

// === bad_variant_access
class bad_variant_access : public ::std::exception {
public:
    bad_variant_access() = default;
    inline const char* what() const noexcept override {
        return "Bad variant access. Too bad!";
    }
};

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
    static constexpr ::std::size_t size = sizeof...(Ts) + 1;

    T v0;
    variant_storage<Ts...> v1;
    
    constexpr variant_storage_impl() noexcept : v1{} {}
};

template<typename T, typename... Ts>
union variant_storage_impl<false, T, Ts...> {
    static constexpr ::std::size_t size = sizeof...(Ts) + 1;

    T v0;
    variant_storage<Ts...> v1;
    
    constexpr variant_storage_impl() noexcept : v1{} {}
    ~variant_storage_impl() {} // TODO: c++20, constexpr
};

// === internal get
template<typename Lhs_T, typename Rhs_T>
struct ddispatch_var_pair {
    using underlying_t = remove_cvref_t<Lhs_T>;
    
    Lhs_T lhs;
    Rhs_T rhs;
};
template<typename Storage_T>
struct var_size {
    static constexpr ::std::size_t size = Storage_T::size;
};
template<typename Lhs_T, typename Rhs_T>
struct var_size<ddispatch_var_pair<Lhs_T, Rhs_T>> {
    static constexpr ::std::size_t size = remove_cvref_t<Lhs_T>::size;
};

template<typename Lhs_T, typename Rhs_T>
constexpr auto make_ddispatch_pair(Lhs_T&& lhs, Rhs_T&& rhs) {
    return ddispatch_var_pair<decltype(::std::forward<Lhs_T>(lhs)), decltype(::std::forward<Rhs_T>(rhs))>{::std::forward<Lhs_T>(lhs), ::std::forward<Rhs_T>(rhs)};
}

template<::std::size_t I>
struct get_variant_storage {
    template<typename Storage_T>
    static constexpr auto do_get(Storage_T&& storage) {
        return get_variant_storage<I - 1>::do_get(storage.v1);
    }
    template<typename Storage_T>
    static constexpr decltype(auto) do_get_ref(Storage_T&& storage) {
        return get_variant_storage<I - 1>::do_get_ref(storage.v1);
    }
    template<typename Lhs_T, typename Rhs_T>
    static constexpr decltype(auto) do_get_ref(ddispatch_var_pair<Lhs_T, Rhs_T>&& storage_pair) {
        return get_variant_storage<I - 1>::do_get_ref(make_ddispatch_pair(::std::forward<Lhs_T>(storage_pair.lhs).v1, ::std::forward<Rhs_T>(storage_pair.rhs).v1));
    }
};
template<>
struct get_variant_storage<0> {
    template<typename Storage_T>
    static constexpr auto do_get(Storage_T&& storage) {
        return &storage.v0;
    }
    template<typename Storage_T>
    static constexpr decltype(auto) do_get_ref(Storage_T&& storage) {
        return (::std::forward<Storage_T>(storage).v0);
    }
    template<typename Lhs_T, typename Rhs_T>
    static constexpr decltype(auto) do_get_ref(ddispatch_var_pair<Lhs_T, Rhs_T>&& storage_pair) {
        return make_ddispatch_pair(::std::forward<Lhs_T>(storage_pair.lhs).v0, ::std::forward<Rhs_T>(storage_pair.rhs).v0);
    }
};

// === visitor
template<typename Ret_T, typename Is_Sequence_T, typename Visitor_T, typename... Storage_Ts>
struct dispatch_variant_storage;

template<typename Ret_T, ::std::size_t... Is, typename Visitor_T, typename... Storage_Ts>
struct dispatch_variant_storage<Ret_T, ::std::index_sequence<Is...>, Visitor_T, Storage_Ts...> {
    static constexpr Ret_T do_dispatch(Visitor_T visitor, Storage_Ts... vs) {
        ::std::forward<decltype(visitor)>(visitor)(get_variant_storage<Is>::do_get_ref(::std::forward<decltype(vs)>(vs))...);
    }
};

template<typename Ret_T, typename Visitor_T, typename... Storage_Ts, ::std::size_t... Is, ::std::size_t... Sequence_Is>
constexpr auto make_dispatch_table(::std::index_sequence<Is...>, ::std::index_sequence<Sequence_Is...>) {
    using dispatch_t = dispatch_variant_storage<Ret_T, ::std::index_sequence<Sequence_Is..., 0>, Visitor_T, Storage_Ts...>;
    return ::std::array<
        decltype(dispatch_t::do_dispatch)*, sizeof...(Is)
    >{dispatch_variant_storage<Ret_T, ::std::index_sequence<Sequence_Is..., Is>, Visitor_T, Storage_Ts...>::do_dispatch...};
}

template<typename Alt_Indices, typename Src_Type>
struct alt_visitor_arg {};

template<typename... Args>
struct alt_visitor_accumulator {};

template<typename Ret_t, typename Accumulator_T, typename Is_Sequence, typename Visitor_T, typename... Storage_Ts>
struct alt_visitor_table;

template<
    typename Ret_T,
    typename... Acc_Ts,
    ::std::size_t... Is_Sequence,
    typename Visitor_T,
    ::std::size_t... Alt_Indices, typename Src_T,
    typename... Storage_Ts
>
struct alt_visitor_table<
    Ret_T,
    alt_visitor_accumulator<Acc_Ts...>,
    ::std::index_sequence<Is_Sequence...>,
    Visitor_T,
    alt_visitor_arg<::std::index_sequence<Alt_Indices...>, Src_T>,
    Storage_Ts...
>
{
    using next_dispatch_t = alt_visitor_table<
        Ret_T,
        alt_visitor_accumulator<Acc_Ts..., Src_T>,
        ::std::index_sequence<Is_Sequence..., 0>,
        Visitor_T,
        Storage_Ts...
    >;
    using next_dispatch_table_t = decltype(next_dispatch_t::dispatch_table);

    static constexpr ::std::array<next_dispatch_table_t, sizeof...(Alt_Indices)> dispatch_table{
        alt_visitor_table<
            Ret_T,
            alt_visitor_accumulator<Acc_Ts..., Src_T>,
            ::std::index_sequence<Is_Sequence..., Alt_Indices>,
            Visitor_T,
            Storage_Ts...
        >::dispatch_table...
    };
};

template<
    typename Ret_T,
    typename... Acc_Ts,
    ::std::size_t... Is_Sequence,
    typename Visitor_T,
    ::std::size_t... Alt_Indices, typename Src_T
>
struct alt_visitor_table<
    Ret_T,
    alt_visitor_accumulator<Acc_Ts...>,
    ::std::index_sequence<Is_Sequence...>,
    Visitor_T,
    alt_visitor_arg<::std::index_sequence<Alt_Indices...>, Src_T>
>
{
    static constexpr auto dispatch_table = make_dispatch_table<Ret_T, Visitor_T, Acc_Ts..., Src_T>(::std::index_sequence<Alt_Indices...>{}, ::std::index_sequence<Is_Sequence...>{});
};

template<::std::size_t N, ::std::size_t I, typename Dispatch_Table_T>
constexpr auto find_dispatch_entry(::std::integral_constant<::std::size_t, I>, const ::std::array<::std::size_t, N>& indices, const Dispatch_Table_T& dispatch_table) {
    return find_dispatch_entry(::std::integral_constant<::std::size_t, I + 1>{}, indices, dispatch_table[I]);
}
template<::std::size_t N, typename Dispatch_Table_T>
constexpr auto find_dispatch_entry(::std::integral_constant<::std::size_t, N>, const ::std::array<::std::size_t, N>& indices, const Dispatch_Table_T& dispatch_table) {
    return dispatch_table;
}

template<typename Ret_T = void, typename Visitor_T, ::std::size_t N, typename... Storage_Ts>
constexpr decltype(auto) visit_impl(Visitor_T&& visitor, const ::std::array<::std::size_t, N>& indices, Storage_Ts&&... vs) {
    using dispatcher_t = alt_visitor_table<
        Ret_T,
        alt_visitor_accumulator<>,
        ::std::index_sequence<>,
        decltype(::std::forward<Visitor_T>(visitor)),
        alt_visitor_arg<::std::make_index_sequence<var_size<remove_cvref_t<Storage_Ts>>::size>, decltype(::std::forward<Storage_Ts>(vs))>...
    >;
    
    constexpr auto dispatch_table = dispatcher_t::dispatch_table;
    const auto dispatch_fun = find_dispatch_entry(::std::integral_constant<::std::size_t, 0>{}, indices, dispatch_table);
    return dispatch_fun(::std::forward<Visitor_T>(visitor), ::std::forward<Storage_Ts>(vs)...);
}

template<typename Ret_T = void, typename Visitor_T, typename Storage_T>
constexpr decltype(auto) visit_impl(Visitor_T&& visitor, ::std::size_t index, Storage_T&& v) {
    return visit_impl<Ret_T>(::std::forward<Visitor_T>(visitor), ::std::array<::std::size_t, 1>{index}, ::std::forward<Storage_T>(v));
}
template<typename Ret_T = void, typename Visitor_T, typename Storage_T1, typename Storage_T2>
constexpr decltype(auto) visit_impl(Visitor_T&& visitor, ::std::size_t index, Storage_T1&& v1, Storage_T2&& v2) {
    const ::std::array<::std::size_t, 1> indices{index};
    return visit_impl<Ret_T>(::std::forward<Visitor_T>(visitor), indices, make_ddispatch_pair(::std::forward<Storage_T1>(v1), ::std::forward<Storage_T2>(v2)));
}

// === generic constructor callables, to be applied with the function above
struct destroy_alternative {
    template<typename Container_T>
    constexpr void operator()(Container_T& value) const {
        value.~Container_T();
    }
};
struct copy_construct_alternative {
    template<typename Pair_T>
    constexpr void operator()(const Pair_T& pair) const {
        ::new (&pair.lhs) typename Pair_T::underlying_t{pair.rhs};
    }
};
struct move_construct_alternative {
    template<typename Pair_T>
    constexpr void operator()(const Pair_T& pair) const {
        ::new (&pair.lhs) typename Pair_T::underlying_t{::std::move(pair.rhs)};
    }
};
struct copy_assign_alternative {
    template<typename Pair_T>
    constexpr void operator()(const Pair_T& pair) const {
        pair.lhs = pair.rhs;
    }
};
struct move_assign_alternative {
    template<typename Pair_T>
    constexpr void operator()(const Pair_T& pair) const {
        pair.lhs = ::std::move(pair.rhs);
    }
};
struct swap_alternative {
    template<typename Pair_T>
    constexpr void operator()(const Pair_T& pair) const {
        using ::std::swap;
        swap(pair.lhs, pair.rhs);
    }
};

struct eq_alternative {
    template<typename Pair_T>
    constexpr bool operator()(const Pair_T& pair) const {
        return pair.lhs == pair.rhs;
    }
};
struct neq_alternative {
    template<typename Pair_T>
    constexpr bool operator()(const Pair_T& pair) const {
        return pair.lhs != pair.rhs;
    }
};
struct less_alternative {
    template<typename Pair_T>
    constexpr bool operator()(const Pair_T& pair) const {
        return pair.lhs < pair.rhs;
    }
};
struct greater_alternative {
    template<typename Pair_T>
    constexpr bool operator()(const Pair_T& pair) const {
        return pair.lhs > pair.rhs;
    }
};
struct leq_alternative {
    template<typename Pair_T>
    constexpr bool operator()(const Pair_T& pair) const {
        return pair.lhs <= pair.rhs;
    }
};
struct geq_alternative {
    template<typename Pair_T>
    constexpr bool operator()(const Pair_T& pair) const {
        return pair.lhs >= pair.rhs;
    }
};
struct hash_alternative {
    template<typename Container_T>
    constexpr ::std::size_t operator()(const Container_T& v) const {
        return ::std::hash<::std::remove_cv<Container_T>>{}(v);
    }
};

// === generic copy/move of two variants
template<typename Var_Lhs, typename Var_Rhs, typename Ctor_Fun, typename Ass_Fun>
constexpr auto& assign_variants(Var_Lhs&& lhs, Var_Rhs&& rhs, Ctor_Fun ctor, Ass_Fun ass) {
    if (rhs._index == variant_npos) {
        if (lhs._index != variant_npos) {
            visit_impl(destroy_alternative{}, lhs._index, lhs._storage);
            lhs._index = variant_npos;
        }
        return lhs;
    }
    
    if (lhs._index == rhs._index) {
        visit_impl(ass, lhs._index, lhs._storage, rhs._storage);
    } else {
        if (lhs._index != variant_npos) {
            visit_impl(destroy_alternative{}, lhs._index, lhs._storage);
            lhs._index = variant_npos;
        }
        
        visit_impl(ctor, rhs._index, lhs._storage, rhs._storage);
        lhs._index = rhs._index;
    }
    return lhs;
}

// === variant_base
template<typename... Ts>
struct variant_base {
    static constexpr ::std::size_t size = sizeof...(Ts);

    variant_storage<Ts...> _storage;
    ::std::size_t _index = variant_npos;
};

// MSVC's __is_constructible struggles with a sfinaed out default ctor, fix below
template<typename... Ts>
constexpr bool var_def_ctor_nothrow_v = ::std::is_nothrow_default_constructible<_internal::first_of_t<Ts...>>::value;

template<bool, typename...>
struct variant_base_def_ctor;

template<typename T, typename... Ts>
struct variant_base_def_ctor<true, T, Ts...> : variant_base<T, Ts...> {
    constexpr variant_base_def_ctor() noexcept(var_def_ctor_nothrow_v<T>) {
        ::new (&this->_storage.v0) T{};
        this->_index = 0;
    }
};

template<typename... Ts>
struct variant_base_def_ctor<false, Ts...> : variant_base<Ts...> {
    variant_base_def_ctor() = delete;
};

/*
 * For each base, the first bool is:
 * - true for trivial OR deleted
 * - false otherwise(non-trivial)
 */
// === variant_base_dtor
template<bool, typename...>
struct variant_base_dtor;

template<typename T, typename... Ts>
struct variant_base_dtor<true, T, Ts...> : variant_base_def_ctor<::std::is_default_constructible<T>::value, T, Ts...> {};

template<typename... Ts>
struct variant_base_dtor<false, Ts...> : variant_base_dtor<true, Ts...> {
    ~variant_base_dtor() { // TODO: c++20 constexpr
        if (this->_index != variant_npos)
            visit_impl(destroy_alternative{}, this->_index, this->_storage);
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
    constexpr variant_base_copy_ctor() = default;
    constexpr variant_base_copy_ctor(const variant_base_copy_ctor& other) {
        if (other._index == variant_npos)
            return;
        visit_impl(copy_construct_alternative{}, other._index, this->_storage, other._storage);
        this->_index = other._index;
    }
};

// === variant_base_move_ctor
template<bool, typename...>
struct variant_base_move_ctor;

template<typename... Ts>
struct variant_base_move_ctor<true, Ts...> : variant_base_copy_ctor<
    conjunction_v<::std::is_trivially_copy_constructible<Ts>...> || !conjunction_v<::std::is_copy_constructible<Ts>...>,
    Ts...
>
{
    constexpr variant_base_move_ctor() noexcept(var_def_ctor_nothrow_v<Ts...>) = default;
    constexpr variant_base_move_ctor(const variant_base_move_ctor&) = default;
    constexpr variant_base_move_ctor(variant_base_move_ctor&&) noexcept(conjunction_v<::std::is_nothrow_move_constructible<Ts>...>) = default;
};

template<typename... Ts>
struct variant_base_move_ctor<false, Ts...> : variant_base_move_ctor<true, Ts...> {
    constexpr variant_base_move_ctor() noexcept(var_def_ctor_nothrow_v<Ts...>) = default;
    constexpr variant_base_move_ctor(const variant_base_move_ctor&) = default;
    constexpr variant_base_move_ctor(variant_base_move_ctor&& other) noexcept(conjunction_v<::std::is_nothrow_move_constructible<Ts>...>) {
        if (other._index == variant_npos)
            return;
        visit_impl(move_construct_alternative{}, other._index, this->_storage, other._storage);
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
    constexpr variant_base_copy_ass() noexcept(var_def_ctor_nothrow_v<Ts...>) = default;
    constexpr variant_base_copy_ass(const variant_base_copy_ass&) = default;
    constexpr variant_base_copy_ass(variant_base_copy_ass&&) noexcept(conjunction_v<::std::is_nothrow_move_constructible<Ts>...>) = default;

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
    
    constexpr variant_base_move_ass() noexcept(var_def_ctor_nothrow_v<Ts...>) = default;
    constexpr variant_base_move_ass(const variant_base_move_ass&) = default;
    constexpr variant_base_move_ass(variant_base_move_ass&&) noexcept(conjunction_v<::std::is_nothrow_move_constructible<Ts>...>) = default;
    constexpr variant_base_move_ass& operator=(const variant_base_move_ass&) = default;
    constexpr variant_base_move_ass& operator=(variant_base_move_ass&&) noexcept(ass_nothrow) = default;
};

template<typename... Ts>
struct variant_base_move_ass<false, Ts...> : variant_base_move_ass<true, Ts...> {
    constexpr variant_base_move_ass() noexcept(var_def_ctor_nothrow_v<Ts...>) = default;
    constexpr variant_base_move_ass(const variant_base_move_ass&) = default;
    constexpr variant_base_move_ass(variant_base_move_ass&&) noexcept(conjunction_v<::std::is_nothrow_move_constructible<Ts>...>) = default;
    constexpr variant_base_move_ass& operator=(const variant_base_move_ass&) = default;

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

template<::std::size_t, typename, typename>
struct alternative_to_index_helper;

template<::std::size_t I, typename Target_T, typename... Ts>
struct alternative_to_index_helper<I, Target_T, variant_base<Ts...>> : alternative_to_index<I, Target_T, Ts...> {};

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

// === type_occurrence_count
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

template<typename, typename>
struct type_occurrence_count_helper;

template<typename T, typename... Ts>
struct type_occurrence_count<T, variant_base<Ts...>> : type_occurrence_count<T, Ts...> {};

// === alternative constructor
template<::std::size_t I, typename... Alt_Ts, typename... Ts>
constexpr auto* init_alternative_at(variant_base<Alt_Ts...>& storage, Ts&&... args) {
    auto* alt_ptr = get_variant_storage<I>::do_get(storage._storage);
    ::new (alt_ptr) ::std::remove_pointer_t<decltype(alt_ptr)>(std::forward<Ts>(args)...);
    storage._index = I;
    return alt_ptr;
}

// === get_impl
template<::std::size_t I, typename Variant_T>
constexpr auto get_if_impl(Variant_T&& v) {
    static_assert(I < v.size, "I is required to be less than the alternative count");
    return v._index == I ? get_variant_storage<I>::do_get(v._storage) : nullptr;
}
template<typename T, typename Variant_T>
constexpr auto get_if_impl(Variant_T&& v) {
    static_assert(type_occurrence_count_helper<T, remove_cvref_t<Variant_T>>::value == 1, "T has occur in Ts exactly once");
    return get_if_impl<alternative_to_index_helper<0, T, remove_cvref_t<Variant_T>>::value>(::std::forward<Variant_T>(v));
}

template<::std::size_t I, typename Variant_T>
constexpr auto get_impl(Variant_T&& v) {
    if (auto ptr = get_if_impl<I>(::std::forward<Variant_T>(v)))
        return ptr;
    throw bad_variant_access{};
}
template<typename T, typename Variant_T>
constexpr auto get_impl(Variant_T&& v) {
    if (auto ptr = get_if_impl<T>(::std::forward<Variant_T>(v)))
        return ptr;
    throw bad_variant_access{};
}

// === relational tests
template<typename, typename = void>
struct eq_test : ::std::false_type {};
template<typename T>
struct eq_test<T, void_t<decltype(static_cast<bool>(::std::declval<T>() == ::std::declval<T>()))>> : ::std::true_type {};

template<typename, typename = void>
struct neq_test : ::std::false_type {};
template<typename T>
struct neq_test<T, void_t<decltype(static_cast<bool>(::std::declval<T>() != ::std::declval<T>()))>> : ::std::true_type {};

template<typename, typename = void>
struct less_test : ::std::false_type {};
template<typename T>
struct less_test<T, void_t<decltype(static_cast<bool>(::std::declval<T>() < ::std::declval<T>()))>> : ::std::true_type {};

template<typename, typename = void>
struct greater_test : ::std::false_type {};
template<typename T>
struct greater_test<T, void_t<decltype(static_cast<bool>(::std::declval<T>() > ::std::declval<T>()))>> : ::std::true_type {};

template<typename, typename = void>
struct leq_test : ::std::false_type {};
template<typename T>
struct leq_test<T, void_t<decltype(static_cast<bool>(::std::declval<T>() <= ::std::declval<T>()))>> : ::std::true_type {};

template<typename, typename = void>
struct geq_test : ::std::false_type {};
template<typename T>
struct geq_test<T, void_t<decltype(static_cast<bool>(::std::declval<T>() >= ::std::declval<T>()))>> : ::std::true_type {};

} // <<< internal

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

// === variant
template<typename... Ts>
class variant : _internal::variant_base_final<Ts...> {
    static_assert(sizeof...(Ts) > 0, "Variant must have at least one alternative");
    
public:
    constexpr variant() noexcept(_internal::var_def_ctor_nothrow_v<Ts...>) = default;
    
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
            !::std::is_same<remove_cvref_t<T>, variant>::value,
        bool> = true,
        ::std::enable_if_t<
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
    constexpr variant_alternative_t<I, variant<Ts...>>& emplace(Args&&... args) {
        static_assert(I < sizeof...(Ts), "I is required to be less than the alternative count");
        if (this->_index != variant_npos) {
            _internal::visit_impl(_internal::destroy_alternative{}, this->_index, this->_storage);
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
    constexpr variant_alternative_t<I, variant<Ts...>>& emplace(::std::initializer_list<U> il, Args&&... args) {
        static_assert(I < sizeof...(Ts), "I is required to be less than the alternative count");
        if (this->_index != variant_npos) {
            _internal::visit_impl(_internal::destroy_alternative{}, this->_index, this->_storage);
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
            
            _internal::visit_impl(_internal::move_construct_alternative{}, rhs._index, this->_storage, rhs._storage);
            ::std::swap(this->_index, rhs._index);
            return;
        }

        if (rhs._index == variant_npos) {
            _internal::visit_impl(_internal::move_construct_alternative{}, this->_index, rhs._storage, this->_storage);
            return;
        }

        if (this->_index == rhs._index) {
            _internal::visit_impl(_internal::swap_alternative{}, this->_index, this->_storage, rhs._storage);
            return;
        }
        
        const auto old_lhs_ind = this->_index;
        const auto old_rhs_ind = rhs._index;
        
        variant tmp_rhs{std::move(rhs)};
        _internal::visit_impl(_internal::move_construct_alternative{}, old_lhs_ind, rhs._storage, this->_storage);
        rhs._storage = old_lhs_ind;
        
        _internal::visit_impl(_internal::move_construct_alternative{}, old_rhs_ind, this->_storage, tmp_rhs._storage);
        this->_index = old_rhs_ind;
    }
    
    constexpr decltype(auto) _internal_base() & {
        return static_cast<_internal::variant_base<Ts...>&>(*this);
    }
    constexpr decltype(auto) _internal_base() const & {
        return static_cast<const _internal::variant_base<Ts...>&>(*this);
    }
    constexpr decltype(auto) _internal_base() && {
        return static_cast<_internal::variant_base<Ts...>&&>(*this);
    }
    constexpr decltype(auto) _internal_base() const && {
        return static_cast<const _internal::variant_base<Ts...>&&>(*this);
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

// === holds_alternative
template<typename T, typename... Ts>
constexpr bool holds_alternative(const variant<Ts...>& v) noexcept {
    static_assert(_internal::type_occurrence_count<T, Ts...>::value == 1, "T has occur in Ts exactly once");
    return !v.valueless_by_exception() && v.index() == _internal::alternative_to_index<0, T, Ts...>::value;
}

// === get<I>
template<::std::size_t I, typename... Ts>
constexpr variant_alternative_t<I, variant<Ts...>>& get(variant<Ts...>& v) {
    return *_internal::get_impl<I>(v._internal_base());
}
template<::std::size_t I, typename... Ts>
constexpr variant_alternative_t<I, variant<Ts...>>&& get(variant<Ts...>&& v) {
    return ::std::move(*_internal::get_impl<I>(v._internal_base()));
}
template<::std::size_t I, typename... Ts>
constexpr const variant_alternative_t<I, variant<Ts...>>& get(const variant<Ts...>& v) {
    return *_internal::get_impl<I>(v._internal_base());
}
template<::std::size_t I, typename... Ts>
constexpr const variant_alternative_t<I, variant<Ts...>>&& get(const variant<Ts...>&& v) {
    return ::std::move(*_internal::get_impl<I>(v._internal_base()));
}

// === get<T>
template<typename T, typename... Ts>
constexpr T& get(variant<Ts...>& v) {
    return *_internal::get_impl<T>(v._internal_base());
}
template<typename T, typename... Ts>
constexpr T&& get(variant<Ts...>&& v) {
    return ::std::move(*_internal::get_impl<T>(v._internal_base()));
}
template<typename T, typename... Ts>
constexpr const T& get(const variant<Ts...>& v) {
    return *_internal::get_impl<T>(v._internal_base());
}
template<typename T, typename... Ts>
constexpr const T&& get(const variant<Ts...>&& v) {
    return ::std::move(*_internal::get_impl<T>(v._internal_base()));
}

// === get_if<I>
template<::std::size_t I, typename... Ts>
constexpr ::std::add_pointer_t<variant_alternative_t<I, variant<Ts...>>> get_if(variant<Ts...>* v) noexcept {
    return v ? _internal::get_if_impl<I>(v->_internal_base()) : nullptr;
}
template<::std::size_t I, typename... Ts>
constexpr ::std::add_pointer_t<const variant_alternative_t<I, variant<Ts...>>> get_if(const variant<Ts...>* v) noexcept {
    return v ? _internal::get_if_impl<I>(v->_internal_base()) : nullptr;
}

// === get_if<T>
template<typename T, typename... Ts>
constexpr ::std::add_pointer_t<T> get_if(variant<Ts...>* v) noexcept {
    return v ? _internal::get_if_impl<T>(v->_internal_base()) : nullptr;
}
template<typename T, typename... Ts>
constexpr ::std::add_pointer_t<const T> get_if(const variant<Ts...>* v) noexcept {
    return v ? _internal::get_if_impl<T>(v->_internal_base()) : nullptr;
}

// === variant relational ops
template<typename... Ts, ::std::enable_if_t<conjunction_v<_internal::eq_test<Ts>...>, bool> = true>
constexpr bool operator==(const variant<Ts...>& v, const variant<Ts...>& w) {
    if (v.index() != w.index())
        return false;
    return v.index() == variant_npos || _internal::visit_impl<bool>(_internal::eq_alternative{}, v.index(), v._internal_base()._storage, w._internal_base()._storage);
}
template<typename... Ts, ::std::enable_if_t<conjunction_v<_internal::neq_test<Ts>...>, bool> = true>
constexpr bool operator!=(const variant<Ts...>& v, const variant<Ts...>& w) {
    if (v.index() != w.index())
        return true;
    return v.index() != variant_npos && _internal::visit_impl<bool>(_internal::neq_alternative{}, v.index(), v._internal_base()._storage, w._internal_base()._storage);
}
template<typename... Ts, ::std::enable_if_t<conjunction_v<_internal::less_test<Ts>...>, bool> = true>
constexpr bool operator<(const variant<Ts...>& v, const variant<Ts...>& w) {
    if (v.index() != w.index())
        return false;
    return v.index() == variant_npos || _internal::visit_impl<bool>(_internal::less_alternative{}, v.index(), v._internal_base()._storage, w._internal_base()._storage);
}
template<typename... Ts, ::std::enable_if_t<conjunction_v<_internal::greater_test<Ts>...>, bool> = true>
constexpr bool operator>(const variant<Ts...>& v, const variant<Ts...>& w) {
    if (v.index() != w.index())
        return false;
    return v.index() == variant_npos || _internal::visit_impl<bool>(_internal::greater_alternative{}, v.index(), v._internal_base()._storage, w._internal_base()._storage);
}
template<typename... Ts, ::std::enable_if_t<conjunction_v<_internal::leq_test<Ts>...>, bool> = true>
constexpr bool operator<=(const variant<Ts...>& v, const variant<Ts...>& w) {
    if (v.index() != w.index())
        return false;
    return v.index() == variant_npos || _internal::visit_impl<bool>(_internal::leq_alternative{}, v.index(), v._internal_base()._storage, w._internal_base()._storage);
}
template<typename... Ts, ::std::enable_if_t<conjunction_v<_internal::geq_test<Ts>...>, bool> = true>
constexpr bool operator>=(const variant<Ts...>& v, const variant<Ts...>& w) {
    if (v.index() != w.index())
        return false;
    return v.index() == variant_npos || _internal::visit_impl<bool>(_internal::geq_alternative{}, v.index(), v._internal_base()._storage, w._internal_base()._storage);
}

// TODO: c++20 spaceship

namespace _internal { // >>> internal

template<typename Visitor_T, typename Expected_T, typename Arg_Accum_T, typename... Variant_Ts>
struct verify_invoke_rets;

template<
    typename Visitor_T,
    typename Expected_T,
    typename... Arg_Ts,
    ::std::size_t... Is, typename Var_T,
    typename... Variant_Ts
>
struct verify_invoke_rets<
    Visitor_T,
    Expected_T,
    alt_visitor_accumulator<Arg_Ts...>,
    alt_visitor_arg<::std::index_sequence<Is...>, Var_T>,
    Variant_Ts...
> : conjunction<
    verify_invoke_rets<
        Expected_T,
        alt_visitor_accumulator<Arg_Ts..., decltype(get<Is>(::std::declval<Var_T>()))>,
        Variant_Ts...
    >
...> {};

template<typename Visitor_T, typename Expected_T, typename... Arg_Ts>
struct verify_invoke_rets<Visitor_T, Expected_T, alt_visitor_accumulator<Arg_Ts...>> : ::std::is_same<
    Expected_T,
    decltype(::std::invoke(::std::declval<Visitor_T>(), ::std::declval<Arg_Ts>()...))
> {};

} // <<< internal

// === visit
template<typename Ret_T, typename Visitor, typename... Variants>
constexpr Ret_T visit(Visitor&& vis, Variants&&... vars) {
    bool has_valueless = false;
    
    using dummy_t = bool[];
    static_cast<void>(dummy_t{(has_valueless = has_valueless || vars.valueless_by_exception())...});
    if (has_valueless)
        throw bad_variant_access{};
    
    const ::std::array<::std::size_t, sizeof...(Variants)> indices{vars._internal_base()._index...};
    return _internal::visit_impl<Ret_T>(::std::forward<Visitor>(vis), indices, ::std::forward<Variants>(vars)._internal_base()._storage...);
}
template<typename Visitor, typename... Variants>
constexpr decltype(auto) visit(Visitor&& vis, Variants&&... vars) {
    using ret_t = decltype(::std::invoke(::std::declval<Visitor>(), get<0>(::std::declval<Variants>()...)));
    static_assert(_internal::verify_invoke_rets<Visitor, ret_t, _internal::alt_visitor_accumulator<>, Variants...>::value, "All invoke results have to match");
    return visit<ret_t>(::std::forward<Visitor>(vis), ::std::forward<Variants>(vars)...);
}

namespace _internal { // >>> internal

template<typename, typename = void>
struct variant_hash_base {};

template<typename... Ts>
struct variant_hash_base<
    variant<Ts...>,
    ::std::enable_if_t<
        conjunction_v<::std::is_copy_constructible<::std::hash<::std::remove_cv_t<Ts>>>...> &&
        conjunction_v<::std::is_destructible<::std::hash<::std::remove_cv_t<Ts>>>...> &&
        conjunction_v<::std::is_copy_assignable<::std::hash<::std::remove_cv_t<Ts>>>...> &&
        conjunction_v<is_swappable<::std::hash<::std::remove_cv_t<Ts>>>...> &&
        conjunction_v<::std::is_same<::std::size_t, decltype(::std::hash<::std::remove_cv<Ts>>{}(::std::declval<Ts>()))...>>
    >
>
{
    constexpr ::std::size_t operator()(const variant<Ts...>& v) const {
        if (v.valueless_by_exception())
            return 0;
        return visit<::std::size_t>(hash_alternative{}, v);
    }
};

} // <<< internal

}

namespace std { // >>> std

// === swap
template<typename... Ts,
    ::std::enable_if_t<
        ::idym::conjunction_v<::std::is_move_constructible<Ts>...> &&
        ::idym::conjunction_v<::idym::is_swappable<Ts...>>,
    bool> = true
>
constexpr void swap(::idym::variant<Ts...>& lhs, ::idym::variant<Ts...>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
}

template<typename... Ts>
struct hash<::idym::variant<Ts...>> : ::idym::_internal::variant_hash_base<::idym::variant<Ts...>> {};

template<>
struct hash<::idym::monostate> {
    inline constexpr ::std::size_t operator()(::idym::monostate) const {
        return 0;
    }
};

} // <<< std

#endif
