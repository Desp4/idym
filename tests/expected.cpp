#include <idym/expected.hpp>

#include "idym_test.hpp"

// common types
struct int_constructible {
    int_constructible() = delete;
    int_constructible(const int& other) : value{other + 1} {}
    int_constructible(int&& other) : value{other + 2} {}

    int value;
};
struct int_constructible_throws {
    int_constructible_throws() = delete;
    int_constructible_throws(const int&) { throw idym_test::test_exception{}; }
    int_constructible_throws(int&&) { throw idym_test::test_exception{}; }
};
struct il_constructible {
    il_constructible() = delete;
    il_constructible(int, void*) : value{123} {}
    il_constructible(std::initializer_list<int>, void*) : value{124} {}

    il_constructible(void*, int) { throw idym_test::test_exception{}; }
    il_constructible(std::initializer_list<void*>, int) { throw idym_test::test_exception{}; }
    
    int value;
};
struct value_type {
    value_type(int val) : value{val} {}
    value_type(void*) { throw idym_test::test_exception{}; }
    value_type(const value_type& other) : value{other.value + 1} {}
    value_type(value_type&& other) : value{other.value + 2} {}
    
    int value = 0;
};
struct destructible {
    destructible(bool* fl) : flag{fl} {}
    ~destructible() { *flag = true; }
    
    bool* flag;
};
struct int_ctor {
    int_ctor() = default;
    int_ctor(int v) : value{v} {}
    
    int value = 0;
};

template<bool Nothrow_Move, bool Noexcept_Copy = false>
struct value_type2 {
    value_type2() = default;
    value_type2(const value_type2&) noexcept(Noexcept_Copy) : value{123} {}
    value_type2(value_type2&&) noexcept(Nothrow_Move) : value{124} {}
    
    value_type2& operator=(const value_type2&) noexcept(Noexcept_Copy) { value = 125; return *this; }
    value_type2& operator=(value_type2&&) noexcept(Nothrow_Move) { value = 126; return *this; }
    
    int value = 0;
};

struct swap_throws {};
void swap(swap_throws&, swap_throws&) noexcept(false) {}

// common traits
template<typename T, typename E>
constexpr bool ex_default_cons_v = std::is_default_constructible<idym::expected<T, E>>::value;
template<typename T, typename E>
constexpr bool ex_copy_cons_v = std::is_copy_constructible<idym::expected<T, E>>::value;
template<typename T, typename E>
constexpr bool ex_trivial_copy_cons = std::is_trivially_copy_constructible<idym::expected<T, E>>::value;
template<typename T, typename E>
constexpr bool ex_move_cons_v = std::is_move_constructible<idym::expected<T, E>>::value;
template<typename T, typename E>
constexpr bool ex_trivial_move_cons_v = std::is_trivially_move_constructible<idym::expected<T, E>>::value;
template<typename T, typename E>
constexpr bool ex_nothrow_move_cons_v = std::is_nothrow_move_constructible<idym::expected<T, E>>::value;

template<typename T, typename E, typename... Ts>
constexpr bool ex_cons_v = std::is_constructible<idym::expected<T, E>, Ts...>::value;

template<typename T, typename E>
constexpr bool ex_trivial_dtor_v = std::is_trivially_destructible<idym::expected<T, E>>::value;

template<typename Expr_T, typename Expected_T>
constexpr bool ex_expr_ret_v = std::is_same<Expr_T, Expected_T>::value;

// common utils
template<typename T, bool Rvalue>
using fwd_assignment_t = std::conditional_t<Rvalue, T&&, T&>;

// [expected.un.cons]
namespace expected_un_cons {
void run_1_9() {
    struct ctor_throws {
        ctor_throws(int) { throw idym_test::test_exception{}; }
        ctor_throws(std::initializer_list<double>) { throw idym_test::test_exception{}; }
    };
    struct multiarg_ctor {
        multiarg_ctor(int aa, double bb) : a{aa}, b{bb} {}
        multiarg_ctor(std::initializer_list<int> list) : a{list.begin()[0] + 1}, b{static_cast<double>(list.begin()[1] + 1)} {}
        
        int a;
        double b;
    };

    static_assert(std::is_constructible<idym::unexpected<double>, int>::value, "expected.un.cons.1.3");
    static_assert(!std::is_constructible<idym::unexpected<double>, void*>::value, "expected.un.cons.1.3");
    static_assert(std::is_constructible<idym::unexpected<multiarg_ctor>, idym::in_place_t, float, double>::value, "expected.un.cons.4");
    static_assert(!std::is_constructible<idym::unexpected<multiarg_ctor>, idym::in_place_t, void*, double>::value, "expected.un.cons.4");
    static_assert(std::is_constructible<idym::unexpected<multiarg_ctor>, idym::in_place_t, std::initializer_list<int>>::value, "expected.un.cons.7");
    static_assert(!std::is_constructible<idym::unexpected<multiarg_ctor>, idym::in_place_t, std::initializer_list<void*>>::value, "expected.un.cons.7");

    {
        const idym::unexpected<double> unex{123};
        idym_test::validate(unex.error() == 123, "expected.un.cons.2");
    }
    IDYM_VALIDATE_EXCEPTION("expected.un.cons.3", idym::unexpected<ctor_throws>{0});
    
    {
        const idym::unexpected<multiarg_ctor> unex{idym::in_place, 4, 2.0};
        idym_test::validate(unex.error().a == 4, "expected.un.cons.5");
        idym_test::validate(unex.error().b == 2.0, "expected.un.cons.5");
    }
    IDYM_VALIDATE_EXCEPTION("expected.un.cons.6", idym::unexpected<ctor_throws>{idym::in_place, 0});
    
    {
        const idym::unexpected<multiarg_ctor> unex{idym::in_place, {1, 2}};
        idym_test::validate(unex.error().a == 2, "expected.un.cons.8");
        idym_test::validate(unex.error().b == 3, "expected.un.cons.8");
    }
    IDYM_VALIDATE_EXCEPTION("expected.un.cons.9", idym::unexpected<ctor_throws>{idym::in_place, {1.0, 2.0}});
}
}

// [expected.un.obs]
namespace expected_un_obs {
void run_1_2() {
    idym::unexpected<int> unex{123};
    
    static_assert(std::is_same<decltype(unex.error()), int&>::value, "expected.un.obs.1");
    static_assert(std::is_same<decltype(idym::as_const(unex).error()), const int&>::value, "expected.un.obs.1");
    static_assert(std::is_same<decltype(std::move(unex).error()), int&&>::value, "expected.un.obs.2");
    static_assert(std::is_same<decltype(std::move(idym::as_const(unex)).error()), const int&&>::value, "expected.un.obs.2");
    
    idym_test::validate(unex.error() == 123, "expected.un.obs.1");
    idym_test::validate(idym::as_const(unex).error() == 123, "expected.un.obs.1");
    idym_test::validate(std::move(unex).error() == 123, "expected.un.obs.1");
    idym_test::validate(std::move(idym::as_const(unex)).error() == 123, "expected.un.obs.1");
}
}

// [expected.un.swap]
namespace expected_un_swap {
void run_1_4() {
    {
        idym::unexpected<int> unex1{123};
        idym::unexpected<int> unex2{456};
    
        unex1.swap(unex2);
        idym_test::validate(unex1.error() == 456, "expected.un.swap.2");
        idym_test::validate(unex2.error() == 123, "expected.un.swap.2");
        
        swap(unex1, unex2);
        idym_test::validate(unex1.error() == 123, "expected.un.swap.4");
        idym_test::validate(unex2.error() == 456, "expectec.un.swap.4");
    }
    
    static_assert(idym::is_swappable_v<idym::unexpected<int>>, "expected.un.swap.4");
}
}

// [expected.un.eq]
namespace expected_un_eq {
void run_1_2() {
    {
        idym::unexpected<int> unex1{123};
        idym::unexpected<int> unex2{456};
        idym::unexpected<int> unex3{123};
        
        idym_test::validate(unex1 == unex3, "expected.un.eq.2");
        idym_test::validate(!(unex1 == unex2), "expected.un.eq.2");
        idym_test::validate(!(unex1 != unex3), "expected.un.eq.2");
        idym_test::validate(unex1 != unex2, "expected.un.eq.2");
    }
}
}

// [expected.object.cons]
namespace expected_object_cons {
void run_1_5() {
    static_assert(!ex_default_cons_v<idym_test::ndef_ctor, int>, "expected.object.cons.2");
    static_assert(ex_default_cons_v<idym_test::def_ctor, int>, "expected.object.cons.2");
    static_assert(ex_default_cons_v<idym_test::def_ctor_throws, int>, "expected.object.cons.2");

    {
        const idym::expected<idym_test::def_ctor, int> ex;
        idym_test::validate(ex.has_value(), "expected.object.cons.4");
        idym_test::validate(ex->value == 1337, "expected.object.cons.3");
    }
    IDYM_VALIDATE_EXCEPTION("expected.object.cons.5", idym::expected<idym_test::def_ctor_throws, int>{});
}
void run_6_10() {
    static_assert(ex_copy_cons_v<idym_test::copy_ctor, idym_test::copy_ctor_throws>, "expected.object.cons.9");
    static_assert(!ex_copy_cons_v<idym_test::ncopy_ctor, int>, "expected.object.cons.9");
    static_assert(!ex_copy_cons_v<int, idym_test::ncopy_ctor>, "expected.object.cons.9");
    static_assert(!ex_copy_cons_v<idym_test::ncopy_ctor, idym_test::ncopy_ctor>, "expected.object.cons.9");

    static_assert(ex_trivial_copy_cons<int, int>, "expected.object.cons.10");
    static_assert(!ex_trivial_copy_cons<idym_test::copy_ctor, int>, "expected.object.cons.10");
    static_assert(!ex_trivial_copy_cons<int, idym_test::copy_ctor>, "expected.object.cons.10");
    static_assert(!ex_trivial_copy_cons<idym_test::copy_ctor, idym_test::copy_ctor>, "expected.object.cons.10");
    
    {
        idym::expected<idym_test::copy_ctor, int> ex1{};
        auto ex2 = ex1;
        
        idym_test::validate(ex2.has_value(), "expected.object.cons.7");
        idym_test::validate(ex2.has_value() == ex1.has_value(), "expected.object.cons.7");
        idym_test::validate(ex2->value == 1, "expected.object.cons.6");
    }
    {
        idym::expected<int, idym_test::copy_ctor> ex1{idym::unexpect};
        auto ex2 = ex1;
        
        idym_test::validate(!ex2.has_value(), "expected.object.cons.7");
        idym_test::validate(ex2.has_value() == ex1.has_value(), "expected.object.cons.7");
        idym_test::validate(ex2.error().value == 1, "expected.object.cons.6");
    }
    {
        idym::expected<idym_test::copy_ctor_throws, int> ex1{};
        IDYM_VALIDATE_EXCEPTION("expected.object.cons.9", idym::expected<idym_test::copy_ctor_throws, int>{ex1});
    }
    {
        idym::expected<int, idym_test::copy_ctor_throws> ex1{idym::unexpect};
        IDYM_VALIDATE_EXCEPTION("expected.object.cons.9", idym::expected<int, idym_test::copy_ctor_throws>{ex1});
    }
}
void run_11_16() {
    static_assert(ex_move_cons_v<idym_test::move_ctor, idym_test::move_ctor_throws>, "expected.object.cons.11");
    static_assert(!ex_move_cons_v<idym_test::nmove_ctor, int>, "expected.object.cons.11");
    static_assert(!ex_move_cons_v<int, idym_test::nmove_ctor>, "expected.object.cons.11");
    static_assert(!ex_move_cons_v<idym_test::nmove_ctor, idym_test::nmove_ctor>, "expected.object.cons.11");
    
    static_assert(ex_trivial_move_cons_v<int, int>, "expected.object.cons.16");
    static_assert(!ex_trivial_move_cons_v<idym_test::move_ctor, int>, "expected.object.cons.16");
    static_assert(!ex_trivial_move_cons_v<int, idym_test::move_ctor>, "expected.object.cons.16");
    static_assert(!ex_trivial_move_cons_v<idym_test::move_ctor, idym_test::move_ctor>, "expected.object.cons.16");
    
    static_assert(ex_nothrow_move_cons_v<idym_test::move_ctor, idym_test::move_ctor>, "expected.object.cons.16");
    static_assert(!ex_nothrow_move_cons_v<idym_test::move_ctor_throws, idym_test::move_ctor>, "expected.object.cons.16");
    static_assert(!ex_nothrow_move_cons_v<idym_test::move_ctor, idym_test::move_ctor_throws>, "expected.object.cons.16");
    static_assert(!ex_nothrow_move_cons_v<idym_test::move_ctor_throws, idym_test::move_ctor_throws>, "expected.object.cons.16");
    
    {
        idym::expected<idym_test::move_ctor, int> ex1{};
        auto ex2 = std::move(ex1);
        
        idym_test::validate(ex2.has_value(), "expected.object.cons.13");
        idym_test::validate(ex2.has_value() == ex1.has_value(), "expected.object.cons.13");
        idym_test::validate(ex2->value == 1, "expected.object.cons.12");
    }
    {
        idym::expected<int, idym_test::move_ctor> ex1{idym::unexpect};
        auto ex2 = std::move(ex1);
        
        idym_test::validate(!ex2.has_value(), "expected.object.cons.13");
        idym_test::validate(ex2.has_value() == ex1.has_value(), "expected.object.cons.13");
        idym_test::validate(ex2.error().value == 1, "expected.object.cons.12");
    }
    {
        idym::expected<idym_test::move_ctor_throws, int> ex1{};
        IDYM_VALIDATE_EXCEPTION("expected.object.cons.14", idym::expected<idym_test::move_ctor_throws, int>{std::move(ex1)});
    }
    {
        idym::expected<int, idym_test::move_ctor_throws> ex1{idym::unexpect};
        IDYM_VALIDATE_EXCEPTION("expected.object.cons.14", idym::expected<int, idym_test::move_ctor_throws>{std::move(ex1)});
    }
}
void run_17_22() {
    static_assert(ex_cons_v<int, double, idym::expected<short, float>&&>, "expected.object.cons.18");
    static_assert(!ex_cons_v<int, double, idym::expected<void*, float>&&>, "expected.object.cons.18");
    
    static_assert(ex_cons_v<int, double, const idym::expected<short, float>&>, "expected.object.cons.18");
    static_assert(!ex_cons_v<int, double, const idym::expected<void*, float>&>, "expected.object.cons.18");
    
    
    {
        const idym::expected<int, int> ex1{123};
        const idym::expected<int_constructible, int> ex2{ex1};
        
        idym_test::validate(ex1.has_value(), "expected.object.cons.20");
        idym_test::validate(ex1.has_value() == ex2.has_value(), "expected.object.cons.20");
        idym_test::validate(ex2->value == 124, "expected.object.cons.19");
        IDYM_VALIDATE_EXCEPTION("expected.object.cons.21", idym::expected<int_constructible_throws, int>{ex1});
    }
    {
        const idym::expected<int, int> ex1{idym::unexpect, 123};
        const idym::expected<int, int_constructible> ex2{ex1};
        
        idym_test::validate(!ex1.has_value(), "expected.object.cons.20");
        idym_test::validate(ex1.has_value() == ex2.has_value(), "expected.object.cons.20");
        idym_test::validate(ex2.error().value == 124, "expected.object.cons.19");
        IDYM_VALIDATE_EXCEPTION("expected.object.cons.21", idym::expected<int, int_constructible_throws>{ex1});
    }
    {
        idym::expected<int, int> ex1{123};
        const idym::expected<int_constructible, int> ex2{std::move(ex1)};
        
        idym_test::validate(ex1.has_value(), "expected.object.cons.20");
        idym_test::validate(ex1.has_value() == ex2.has_value(), "expected.object.cons.20");
        idym_test::validate(ex2->value == 125, "expected.object.cons.19");
        IDYM_VALIDATE_EXCEPTION("expected.object.cons.21", idym::expected<int_constructible_throws, int>{std::move(ex1)});
    }
    {
        idym::expected<int, int> ex1{idym::unexpect, 123};
        const idym::expected<int, int_constructible> ex2{std::move(ex1)};
        
        idym_test::validate(!ex1.has_value(), "expected.object.cons.20");
        idym_test::validate(ex1.has_value() == ex2.has_value(), "expected.object.cons.20");
        idym_test::validate(ex2.error().value == 125, "expected.object.cons.19");
        IDYM_VALIDATE_EXCEPTION("expected.object.cons.21", idym::expected<int, int_constructible_throws>{std::move(ex1)});
    }
}
void run_23_31() {
    static_assert(ex_cons_v<int, void*, int>, "expected.object.cons.23");
    static_assert(ex_cons_v<int, void*, double>, "expected.object.cons.23");
    static_assert(!ex_cons_v<int, void*, void*>, "expected.object.cons.23");
    
    {
        const value_type value{123};
        const idym::expected<value_type, int> ex{value};
        idym_test::validate(ex.has_value(), "expected.object.cons.25");
        idym_test::validate(ex->value == 124, "expected.object.cons.24");
    }
    {
        value_type value{123};
        const idym::expected<value_type, int> ex{std::move(value)};
        idym_test::validate(ex.has_value(), "expected.object.cons.25");
        idym_test::validate(ex->value == 125, "expected.object.cons.24");
    }
    IDYM_VALIDATE_EXCEPTION("expected.object.cons.26", idym::expected<value_type, int>{nullptr});
    
    static_assert(ex_cons_v<void*, int, idym::unexpected<int>>, "expected.object.cons.28");
    static_assert(ex_cons_v<void*, int, idym::unexpected<double>>, "expected.object.cons.28");
    static_assert(!ex_cons_v<void*, int, idym::unexpected<void*>>, "expected.object.cons.28");

    {
        const idym::unexpected<value_type> value{123};
        const idym::expected<int, value_type> ex{value};
        idym_test::validate(!ex.has_value(), "expected.object.cons.30");
        idym_test::validate(ex.error().value == 124, "expected.object.cons.29");
    }
    {
        idym::unexpected<value_type> value{123};
        const idym::expected<int, value_type> ex{std::move(value)};
        idym_test::validate(!ex.has_value(), "expected.object.cons.30");
        idym_test::validate(ex.error().value == 125, "expected.object.cons.29");
    }
    IDYM_VALIDATE_EXCEPTION("expected.object.cons.31", idym::expected<int, value_type>{idym::unexpected<void*>{nullptr}});
}
void run_32_43() {
    static_assert(ex_cons_v<il_constructible, int, idym::in_place_t, int, void*>, "expected.object.cons.32");
    static_assert(!ex_cons_v<il_constructible, int, idym::in_place_t, int, int, void*>, "expected.object.cons.32");
    static_assert(ex_cons_v<il_constructible, int, idym::in_place_t, std::initializer_list<int>, void*>, "expected.object.cons.36");
    static_assert(!ex_cons_v<il_constructible, int, idym::in_place_t, std::initializer_list<int>, void*, void*>, "expected.object.cons.36");
    
    {
        const idym::expected<il_constructible, int> ex{idym::in_place, 1, nullptr};
        idym_test::validate(ex.has_value(), "expected.object.cons.34");
        idym_test::validate(ex->value == 123, "expected.object.cons.33");
    }
    {
        const idym::expected<il_constructible, int> ex{idym::in_place, {1}, nullptr};
        idym_test::validate(ex.has_value(), "expected.object.cons.38");
        idym_test::validate(ex->value == 124, "expected.object.cons.37");
    }
    IDYM_VALIDATE_EXCEPTION("expected.object.cons.35", idym::expected<il_constructible, int>{idym::in_place, nullptr, 1});
    IDYM_VALIDATE_EXCEPTION("expected.object.cons.39", idym::expected<il_constructible, int>{idym::in_place, {static_cast<void*>(nullptr)}, 1});

    static_assert(ex_cons_v<int, il_constructible, idym::unexpect_t, int, void*>, "expected.object.cons.40");
    static_assert(!ex_cons_v<int, il_constructible, idym::unexpect_t, int, int, void*>, "expected.object.cons.40");
    static_assert(ex_cons_v<int, il_constructible, idym::unexpect_t, std::initializer_list<int>, void*>, "expected.object.cons.44");
    static_assert(!ex_cons_v<int, il_constructible, idym::unexpect_t, std::initializer_list<int>, void*, void*>, "expected.object.cons.44");
    
    {
        const idym::expected<int, il_constructible> ex{idym::unexpect, 1, nullptr};
        idym_test::validate(!ex.has_value(), "expected.object.cons.42");
        idym_test::validate(ex.error().value == 123, "expected.object.cons.41");
    }
    {
        const idym::expected<int, il_constructible> ex{idym::unexpect, {1}, nullptr};
        idym_test::validate(!ex.has_value(), "expected.object.cons.46");
        idym_test::validate(ex.error().value == 124, "expected.object.cons.45");
    }
    IDYM_VALIDATE_EXCEPTION("expected.object.cons.43", idym::expected<int, il_constructible>{idym::unexpect, nullptr, 1});
    IDYM_VALIDATE_EXCEPTION("expected.object.cons.47", idym::expected<int, il_constructible>{idym::unexpect, {static_cast<void*>(nullptr)}, 1});
}
}

// [expected.object.dtor]
namespace expected_object_dtor {
void run_1_2() {
    static_assert(ex_trivial_dtor_v<int, int>, "expected.object.dtor.2");
    static_assert(!ex_trivial_dtor_v<destructible, int>, "expected.object.dtor.2");
    static_assert(!ex_trivial_dtor_v<int, destructible>, "expected.object.dtor.2");
    static_assert(!ex_trivial_dtor_v<destructible, destructible>, "expected.object.dtor.2");
    
    {
        bool destroyed = false;
        idym::expected<destructible, int>{idym::in_place, &destroyed};
        idym_test::validate(destroyed, "expected.object.dtor.1");
    }
    {
        bool destroyed = false;
        idym::expected<int, destructible>{idym::unexpect, &destroyed};
        idym_test::validate(destroyed, "expected.object.dtor.1");
    }
}
}

// [expected.object.assign]
namespace expected_object_assign {
template<bool Rvalue, bool Nothrow_Move>
void validate_assignment(const char* clause_str, int same_member, int cross_member) {
    {
        idym::expected<value_type2<Nothrow_Move>, value_type2<true>> ex1{idym::in_place};
        idym::expected<value_type2<Nothrow_Move>, value_type2<true>> ex2{idym::in_place};
        
        ex2 = static_cast<fwd_assignment_t<decltype(ex1), Rvalue>>(ex1);
        idym_test::validate(ex2.has_value() == ex1.has_value(), clause_str);
        idym_test::validate(ex2->value == same_member, clause_str);
    }
    {
        idym::expected<value_type2<true>, value_type2<Nothrow_Move>> ex1{idym::unexpect};
        idym::expected<value_type2<true>, value_type2<Nothrow_Move>> ex2{idym::unexpect};
        
        ex2 = static_cast<fwd_assignment_t<decltype(ex1), Rvalue>>(ex1);
        idym_test::validate(ex2.has_value() == ex1.has_value(), clause_str);
        idym_test::validate(ex2.error().value == same_member, clause_str);
    }
    {
        idym::expected<value_type2<Nothrow_Move>, value_type2<true>> ex1{idym::unexpect};
        idym::expected<value_type2<Nothrow_Move>, value_type2<true>> ex2{idym::in_place};
        
        ex2 = static_cast<fwd_assignment_t<decltype(ex1), Rvalue>>(ex1);
        idym_test::validate(ex2.has_value() == ex1.has_value(), clause_str);
        idym_test::validate(ex2.error().value == cross_member, clause_str);
    }
    {
        idym::expected<value_type2<true>, value_type2<Nothrow_Move>> ex1{idym::in_place};
        idym::expected<value_type2<true>, value_type2<Nothrow_Move>> ex2{idym::unexpect};
        
        ex2 = static_cast<fwd_assignment_t<decltype(ex1), Rvalue>>(ex1);
        idym_test::validate(ex2.has_value() == ex1.has_value(), clause_str);
        idym_test::validate(ex2->value == cross_member, clause_str);
    }
}

void run_2_8() {
    struct reinit_throws {
        reinit_throws() = default;
        reinit_throws(reinit_throws&&) { throw idym_test::test_exception{}; };
        
        reinit_throws& operator=(reinit_throws&&) { return *this; }
    };

    validate_assignment<false, true>("expected.object.assign.2", 125, 124);
    validate_assignment<false, false>("expected.object.assign.2", 125, 124);
    validate_assignment<true, true>("expected.object.assign.6", 126, 124);
    validate_assignment<true, false>("expected.object.assign.6", 126, 124);
    
    {
        using nothrow_value_type = value_type2<true, true>;
        idym::expected<nothrow_value_type, nothrow_value_type> ex1{idym::unexpect};
        idym::expected<nothrow_value_type, nothrow_value_type> ex2{idym::in_place};
        
        ex2 = ex1;
        idym_test::validate(ex2.has_value() == ex1.has_value(), "expected.object.assign.2");
        idym_test::validate(ex2->value == 123, "expected.object.assign.2");
    }
    
    {
        idym::expected<reinit_throws, int> ex1{idym::in_place};
        idym::expected<reinit_throws, int> ex2{idym::unexpect};
        
        IDYM_VALIDATE_EXCEPTION("expected.object.assign.6", ex2 = std::move(ex1));
        idym_test::validate(!ex2.has_value(), "expected.object.assign.6");
    }
}
void run_9_15() {
    {
        idym::expected<int_ctor, int> ex{};
        ex = 123;
        
        idym_test::validate(ex.has_value(), "expected.object.assign.10");
        idym_test::validate(ex->value == 123, "expected.object.assign.10");
    }
    {
        idym::expected<int_ctor, int> ex{idym::unexpect};
        ex = 123;
        
        idym_test::validate(ex.has_value(), "expected.object.assign.10");
        idym_test::validate(ex->value == 123, "expected.object.assign.10");
    }
    
    {
        idym::expected<int, int_ctor> ex{};
        ex = idym::unexpected<int>{123};
        
        idym_test::validate(!ex.has_value(), "expected.object.assign.14");
        idym_test::validate(ex.error().value == 123, "expected.object.assign.14");
    }
    {
        idym::expected<int, int_ctor> ex{idym::unexpect};
        ex = idym::unexpected<int>{123};
        
        idym_test::validate(!ex.has_value(), "expected.object.assign.14");
        idym_test::validate(ex.error().value == 123, "expected.object.assign.14");
    }
}
void run_16_19() {
    struct il_ctor {
        il_ctor() = default;
        il_ctor(std::initializer_list<int> il, int v) noexcept : value{il.begin()[0] +  v} {}
        il_ctor(int v1, int v2) noexcept : value{v1 + v2} {}
        
        int value = 0;
    };
    
    {
        idym::expected<il_ctor, int> ex;

        ex.emplace(4, 5);
        idym_test::validate(ex.has_value(), "expected.object.assign.17");
        idym_test::validate(ex->value == 9, "expected.object.assign.17");
    }
    {
        idym::expected<il_ctor, int> ex;
        
        ex.emplace({10}, 20);
        idym_test::validate(ex.has_value(), "expected.object.assign.19");
        idym_test::validate(ex->value == 30, "expected.object.assign.19");
    }
    {
        idym::expected<il_ctor, int> ex{idym::unexpect};

        ex.emplace(4, 5);
        idym_test::validate(ex.has_value(), "expected.object.assign.17");
        idym_test::validate(ex->value == 9, "expected.object.assign.17");
    }
    {
        idym::expected<il_ctor, int> ex{idym::unexpect};
        
        ex.emplace({10}, 20);
        idym_test::validate(ex.has_value(), "expected.object.assign.19");
        idym_test::validate(ex->value == 30, "expected.object.assign.19");
    }
}
}

// [expected.object.swap]
namespace expected_object_swap {
void run_1_5() {
    static_assert(idym::is_nothrow_swappable<idym::expected<int, int>>::value, "expected.object.swap.4");
    static_assert(!idym::is_nothrow_swappable<idym::expected<int, swap_throws>>::value, "expected.object.swap.4");

    {
        idym::expected<int, int> ex1{123};
        idym::expected<int, int> ex2{124};
        
        swap(ex1, ex2);
        idym_test::validate(ex1.has_value(), "expected.object.swap.2");
        idym_test::validate(ex2.has_value(), "expected.object.swap.2");
        idym_test::validate(*ex1 == 124, "expected.object.swap.2");
        idym_test::validate(*ex2 == 123, "expected.object.swap.2");
        
        ex1.swap(ex2);
        idym_test::validate(ex1.has_value(), "expected.object.swap.2");
        idym_test::validate(ex2.has_value(), "expected.object.swap.2");
        idym_test::validate(*ex1 == 123, "expected.object.swap.2");
        idym_test::validate(*ex2 == 124, "expected.object.swap.2");
    }
    {
        idym::expected<int, int> ex1{idym::unexpect, 123};
        idym::expected<int, int> ex2{124};
        
        swap(ex1, ex2);
        idym_test::validate(ex1.has_value(), "expected.object.swap.2");
        idym_test::validate(!ex2.has_value(), "expected.object.swap.2");
        idym_test::validate(*ex1 == 124, "expected.object.swap.2");
        idym_test::validate(ex2.error() == 123, "expected.object.swap.2");
        
        ex1.swap(ex2);
        idym_test::validate(!ex1.has_value(), "expected.object.swap.2");
        idym_test::validate(ex2.has_value(), "expected.object.swap.2");
        idym_test::validate(ex1.error() == 123, "expected.object.swap.2");
        idym_test::validate(*ex2 == 124, "expected.object.swap.2");
    }
    {
        idym::expected<int, int> ex1{123};
        idym::expected<int, int> ex2{idym::unexpect, 124};
        
        swap(ex1, ex2);
        idym_test::validate(!ex1.has_value(), "expected.object.swap.2");
        idym_test::validate(ex2.has_value(), "expected.object.swap.2");
        idym_test::validate(ex1.error() == 124, "expected.object.swap.2");
        idym_test::validate(*ex2 == 123, "expected.object.swap.2");
        
        ex1.swap(ex2);
        idym_test::validate(ex1.has_value(), "expected.object.swap.2");
        idym_test::validate(!ex2.has_value(), "expected.object.swap.2");
        idym_test::validate(*ex1 == 123, "expected.object.swap.2");
        idym_test::validate(ex2.error() == 124, "expected.object.swap.2");
    }
    {
        idym::expected<int, int> ex1{idym::unexpect, 123};
        idym::expected<int, int> ex2{idym::unexpect, 124};
        
        swap(ex1, ex2);
        idym_test::validate(!ex1.has_value(), "expected.object.swap.2");
        idym_test::validate(!ex2.has_value(), "expected.object.swap.2");
        idym_test::validate(ex1.error() == 124, "expected.object.swap.2");
        idym_test::validate(ex2.error() == 123, "expected.object.swap.2");
        
        ex1.swap(ex2);
        idym_test::validate(!ex1.has_value(), "expected.object.swap.2");
        idym_test::validate(!ex2.has_value(), "expected.object.swap.2");
        idym_test::validate(ex1.error() == 123, "expected.object.swap.2");
        idym_test::validate(ex2.error() == 124, "expected.object.swap.2");
    }
}
}

// [expected.object.obs]
namespace expected_object_obs {
void run_1_2() {
    struct int_member {
        int value;
    };

    static_assert(ex_expr_ret_v<decltype(std::declval<idym::expected<int, int>&>().operator->()), int*>, "expected.object.obs.2");
    static_assert(ex_expr_ret_v<decltype(std::declval<const idym::expected<int, int>&>().operator->()), const int*>, "expected.object.obs.2");

    {
        idym::expected<int_member, int> ex{int_member{123}};
        idym_test::validate(ex->value == 123, "expected.object.obs.2");
    }
}
void run_3_6() {
    static_assert(ex_expr_ret_v<decltype(*std::declval<idym::expected<int, int>&>()), int&>, "expected.object.obs.4");
    static_assert(ex_expr_ret_v<decltype(*std::declval<const idym::expected<int, int>&>()), const int&>, "expected.object.obs.4");

    static_assert(ex_expr_ret_v<decltype(*std::declval<idym::expected<int, int>&&>()), int&&>, "expected.object.obs.6");
    static_assert(ex_expr_ret_v<decltype(*std::declval<const idym::expected<int, int>&&>()), const int&&>, "expected.object.obs.6");

    {
        idym::expected<int, int> ex{123};
        idym_test::validate(*ex == 123, "expected.object.obs.4");
        idym_test::validate(*std::move(ex) == 123, "expected.object.obs.6");
    }
}
void run_7() {
    {
        idym::expected<int, int> ex{idym::in_place};
        idym_test::validate(ex.has_value(), "expected.object.obs.7");
        idym_test::validate(static_cast<bool>(ex), "expected.object.obs.7");
    }
    {
        idym::expected<int, int> ex{idym::unexpect};
        idym_test::validate(!ex.has_value(), "expected.object.obs.7");
        idym_test::validate(!static_cast<bool>(ex), "expected.object.obs.7");
    }
}
void run_8_13() {
    static_assert(ex_expr_ret_v<decltype(std::declval<idym::expected<int, int>&>().value()), int&>, "expected.object.obs.9");
    static_assert(ex_expr_ret_v<decltype(std::declval<const idym::expected<int, int>&>().value()), const int&>, "expected.object.obs.9");

    static_assert(ex_expr_ret_v<decltype(std::declval<idym::expected<int, int>&&>().value()), int&&>, "expected.object.obs.12");
    static_assert(ex_expr_ret_v<decltype(std::declval<const idym::expected<int, int>&&>().value()), const int&&>, "expected.object.obs.12");

    {
        idym::expected<int, int> ex{123};
        idym_test::validate(ex.value() == 123, "expected.object.obs.9");
        idym_test::validate(std::move(ex).value() == 123, "expected.object.obs.12");
    }
    {
        idym::expected<int, int> ex{idym::unexpect, 123};

        bool caught = false;
        try {
            ex.value();
        } catch (idym::bad_expected_access<int> v) {
            caught = v.error() == 123;
        }
        idym_test::validate(caught, "expected.object.obs.10");
    }
    {
        idym::expected<int, int> ex{idym::unexpect, 123};

        bool caught = false;
        try {
            std::move(ex).value();
        } catch (idym::bad_expected_access<int> v) {
            caught = v.error() == 123;
        }
        idym_test::validate(caught, "expected.object.obs.13");
    }
}
void run_14_17() {
    static_assert(ex_expr_ret_v<decltype(std::declval<idym::expected<int, int>&>().error()), int&>, "expected.object.obs.15");
    static_assert(ex_expr_ret_v<decltype(std::declval<const idym::expected<int, int>&>().error()), const int&>, "expected.object.obs.15");

    static_assert(ex_expr_ret_v<decltype(std::declval<idym::expected<int, int>&&>().error()), int&&>, "expected.object.obs.17");
    static_assert(ex_expr_ret_v<decltype(std::declval<const idym::expected<int, int>&&>().error()), const int&&>, "expected.object.obs.17");

    {
        idym::expected<int, int> ex{idym::unexpect, 123};
        idym_test::validate(ex.error() == 123, "expected.object.obs.15");
        idym_test::validate(std::move(ex).error() == 123, "expected.object.obs.17");
    }
}
void run_18_25() {
    {
        idym::expected<int, int> ex{idym::in_place, 123};
        idym_test::validate(ex.value_or(124) == 123, "expected.object.obs.19");
        idym_test::validate(ex.error_or(124) == 124, "expected.object.obs.23");

        idym_test::validate(std::move(ex).value_or(124) == 123, "expected.object.obs.21");
        idym_test::validate(std::move(ex).error_or(124) == 124, "expected.object.obs.25");
    }
    {
        idym::expected<int, int> ex{idym::unexpect, 123};
        idym_test::validate(ex.value_or(124) == 124, "expected.object.obs.19");
        idym_test::validate(ex.error_or(124) == 123, "expected.object.obs.23");

        idym_test::validate(std::move(ex).value_or(124) == 124, "expected.object.obs.21");
        idym_test::validate(std::move(ex).error_or(124) == 123, "expected.object.obs.25");
    }
}
}

// [expected.object.monadic]
namespace expected_object_monadic {
idym::expected<short, short> consume_value(int value) {
    return idym::expected<short, short>{idym::in_place, value + 1};
}
idym::expected<long, long> consume_unex(short value) {
    return idym::expected<long, long>{idym::unexpect, value + 2};
}

short consume_value2(int value) {
    return value + 3;
}
long consume_unex2(int value) {
    return value + 4;
}

void consume_void_value(int){}

void run_1_8() {
    {
        idym::expected<int, short> ex{idym::in_place, 123};

        const auto ret1 = ex.and_then(consume_value);
        idym_test::validate(ret1.has_value(), "expected.object.monadic.4");
        idym_test::validate(*ret1 == 124, "expected.object.monadic.4");

        const auto ret2 = std::move(ex).and_then(consume_value);
        idym_test::validate(ret2.has_value(), "expected.object.monadic.8");
        idym_test::validate(*ret2 == 124, "expected.object.monadic.8");

        static_assert(std::is_same<idym::remove_cvref_t<decltype(ret1)>, idym::expected<short, short>>::value, "expected.object.monadic.4");
        static_assert(std::is_same<idym::remove_cvref_t<decltype(ret2)>, idym::expected<short, short>>::value, "expected.object.monadic.8");
    }
    {
        idym::expected<int, short> ex{idym::unexpect, 123};

        const auto ret1 = ex.and_then(consume_value);
        idym_test::validate(!ret1.has_value(), "expected.object.monadic.4");
        idym_test::validate(ret1.error() == 123, "expected.object.monadic.4");

        const auto ret2 = std::move(ex).and_then(consume_value);
        idym_test::validate(!ret2.has_value(), "expected.object.monadic.8");
        idym_test::validate(ret2.error() == 123, "expected.object.monadic.8");

        static_assert(std::is_same<idym::remove_cvref_t<decltype(ret1)>, idym::expected<short, short>>::value, "expected.object.monadic.4");
        static_assert(std::is_same<idym::remove_cvref_t<decltype(ret2)>, idym::expected<short, short>>::value, "expected.object.monadic.8");
    }
}
void run_9_16() {
    {
        idym::expected<int, short> ex{idym::in_place, 123};

        const auto ret1 = ex.or_else(consume_unex);
        idym_test::validate(ret1.has_value(), "expected.object.monadic.12");
        idym_test::validate(*ret1 == 123, "expected.object.monadic.12");

        const auto ret2 = std::move(ex).or_else(consume_unex);
        idym_test::validate(ret2.has_value(), "expected.object.monadic.16");
        idym_test::validate(*ret2 == 123, "expected.object.monadic.16");

        static_assert(std::is_same<idym::remove_cvref_t<decltype(ret1)>, idym::expected<long, long>>::value, "expected.object.monadic.12");
        static_assert(std::is_same<idym::remove_cvref_t<decltype(ret2)>, idym::expected<long, long>>::value, "expected.object.monadic.16");
    }
    {
        idym::expected<int, short> ex{idym::unexpect, 123};

        const auto ret1 = ex.or_else(consume_unex);
        idym_test::validate(!ret1.has_value(), "expected.object.monadic.12");
        idym_test::validate(ret1.error() == 125, "expected.object.monadic.12");

        const auto ret2 = std::move(ex).or_else(consume_unex);
        idym_test::validate(!ret2.has_value(), "expected.object.monadic.16");
        idym_test::validate(ret2.error() == 125, "expected.object.monadic.16");

        static_assert(std::is_same<idym::remove_cvref_t<decltype(ret1)>, idym::expected<long, long>>::value, "expected.object.monadic.12");
        static_assert(std::is_same<idym::remove_cvref_t<decltype(ret2)>, idym::expected<long, long>>::value, "expected.object.monadic.16");
    }
}
void run_17_24() {
    {
        idym::expected<int, short> ex{idym::in_place, 123};

        const auto ret1 = ex.transform(consume_value2);
        idym_test::validate(ret1.has_value(), "expected.object.monadic.20");
        idym_test::validate(*ret1 == 126, "expected.object.monadic.20");

        const auto ret2 = std::move(ex).transform(consume_value2);
        idym_test::validate(ret2.has_value(), "expected.object.monadic.24");
        idym_test::validate(*ret2 == 126, "expected.object.monadic.24");

        static_assert(std::is_same<idym::remove_cvref_t<decltype(ret1)>, idym::expected<short, short>>::value, "expected.object.monadic.20");
        static_assert(std::is_same<idym::remove_cvref_t<decltype(ret2)>, idym::expected<short, short>>::value, "expected.object.monadic.24");
    }
    {
        idym::expected<int, short> ex{idym::unexpect, 123};

        const auto ret1 = ex.transform(consume_value2);
        idym_test::validate(!ret1.has_value(), "expected.object.monadic.20");
        idym_test::validate(ret1.error() == 123, "expected.object.monadic.20");

        const auto ret2 = std::move(ex).transform(consume_value2);
        idym_test::validate(!ret1.has_value(), "expected.object.monadic.24");
        idym_test::validate(ret1.error() == 123, "expected.object.monadic.24");

        static_assert(std::is_same<idym::remove_cvref_t<decltype(ret1)>, idym::expected<short, short>>::value, "expected.object.monadic.20");
        static_assert(std::is_same<idym::remove_cvref_t<decltype(ret2)>, idym::expected<short, short>>::value, "expected.object.monadic.24");
    }
    {
        idym::expected<int, short> ex{idym::in_place, 123};
        
        const auto ret1 = ex.transform(consume_void_value);
        idym_test::validate(ret1.has_value(), "expected.object.monadic.20");
        
        const auto ret2 = std::move(ex).transform(consume_void_value);
        idym_test::validate(ret2.has_value(), "expected.object.monadic.24");
        
        static_assert(std::is_same<idym::remove_cvref_t<decltype(ret1)>, idym::expected<void, short>>::value, "expected.object.monadic.20");
        static_assert(std::is_same<idym::remove_cvref_t<decltype(ret2)>, idym::expected<void, short>>::value, "expected.object.monadic.24");
    }
    {
        idym::expected<int, short> ex{idym::unexpect, 123};

        const auto ret1 = ex.transform(consume_void_value);
        idym_test::validate(!ret1.has_value(), "expected.object.monadic.20");
        idym_test::validate(ret1.error() == 123, "expected.object.monadic.20");

        const auto ret2 = std::move(ex).transform(consume_void_value);
        idym_test::validate(!ret1.has_value(), "expected.object.monadic.24");
        idym_test::validate(ret1.error() == 123, "expected.object.monadic.24");

        static_assert(std::is_same<idym::remove_cvref_t<decltype(ret1)>, idym::expected<void, short>>::value, "expected.object.monadic.20");
        static_assert(std::is_same<idym::remove_cvref_t<decltype(ret2)>, idym::expected<void, short>>::value, "expected.object.monadic.24");
    }
}
void run_25_32() {
    {
        idym::expected<int, short> ex{idym::in_place, 123};

        const auto ret1 = ex.transform_error(consume_unex2);
        idym_test::validate(ret1.has_value(), "expected.object.monadic.28");
        idym_test::validate(*ret1 == 123, "expected.object.monadic.28");

        const auto ret2 = std::move(ex).transform_error(consume_unex2);
        idym_test::validate(ret2.has_value(), "expected.object.monadic.32");
        idym_test::validate(*ret2 == 123, "expected.object.monadic.32");

        static_assert(std::is_same<idym::remove_cvref_t<decltype(ret1)>, idym::expected<int, long>>::value, "expected.object.monadic.28");
        static_assert(std::is_same<idym::remove_cvref_t<decltype(ret2)>, idym::expected<int, long>>::value, "expected.object.monadic.32");
    }
    {
        idym::expected<int, short> ex{idym::unexpect, 123};

        const auto ret1 = ex.transform_error(consume_unex2);
        idym_test::validate(!ret1.has_value(), "expected.object.monadic.28");
        idym_test::validate(ret1.error() == 127, "expected.object.monadic.28");

        const auto ret2 = std::move(ex).transform_error(consume_unex2);
        idym_test::validate(!ret1.has_value(), "expected.object.monadic.32");
        idym_test::validate(ret1.error() == 127, "expected.object.monadic.32");

        static_assert(std::is_same<idym::remove_cvref_t<decltype(ret1)>, idym::expected<int, long>>::value, "expected.object.monadic.28");
        static_assert(std::is_same<idym::remove_cvref_t<decltype(ret2)>, idym::expected<int, long>>::value, "expected.object.monadic.32");
    }
}
}

// [expected.object.eq]
namespace expected_object_eq {
void run_1_2() {
    {
        idym::expected<int, short> ex1{idym::in_place, 123};
        idym::expected<long, char> ex2{idym::in_place, 123};

        idym_test::validate(ex1 == ex2, "expected.object.eq.2");
        idym_test::validate(ex2 == ex2, "expected.object.eq.2");
        idym_test::validate(!(ex1 != ex2), "expected.object.eq.2");
        idym_test::validate(!(ex2 != ex2), "expected.object.eq.2");
    }
    {
        idym::expected<int, short> ex1{idym::in_place, 123};
        idym::expected<long, char> ex2{idym::in_place, 124};

        idym_test::validate(!(ex1 == ex2), "expected.object.eq.2");
        idym_test::validate(!(ex2 == ex1), "expected.object.eq.2");
        idym_test::validate(ex1 != ex2, "expected.object.eq.2");
        idym_test::validate(ex2 != ex1, "expected.object.eq.2");
    }
    {
        idym::expected<int, short> ex1{idym::unexpect, 123};
        idym::expected<long, char> ex2{idym::unexpect, 123};

        idym_test::validate(ex1 == ex2, "expected.object.eq.2");
        idym_test::validate(ex2 == ex1, "expected.object.eq.2");
        idym_test::validate(!(ex1 != ex2), "expected.object.eq.2");
        idym_test::validate(!(ex2 != ex1), "expected.object.eq.2");
    }
    {
        idym::expected<int, short> ex1{idym::unexpect, 123};
        idym::expected<long, char> ex2{idym::unexpect, 124};

        idym_test::validate(!(ex1 == ex2), "expected.object.eq.2");
        idym_test::validate(!(ex2 == ex1), "expected.object.eq.2");
        idym_test::validate(ex1 != ex2, "expected.object.eq.2");
        idym_test::validate(ex2 != ex1, "expected.object.eq.2");
    }
    {
        idym::expected<int, short> ex1{idym::unexpect, 123};
        idym::expected<long, char> ex2{idym::in_place, 124};

        idym_test::validate(!(ex1 == ex2), "expected.object.eq.2");
        idym_test::validate(!(ex2 == ex1), "expected.object.eq.2");
        idym_test::validate(ex1 != ex2, "expected.object.eq.2");
        idym_test::validate(ex2 != ex1, "expected.object.eq.2");
    }
}
void run_3_4() {
    {
        idym::expected<int, short> ex{idym::in_place, 123};
        long value = 123;

        idym_test::validate(ex == value, "expected.object.eq.4");
        idym_test::validate(value == ex, "expected.object.eq.4");
        idym_test::validate(!(ex != value), "expected.object.eq.4");
        idym_test::validate(!(value != ex), "expected.object.eq.4");
    }
    {
        idym::expected<int, short> ex{idym::unexpect, 123};
        long value = 123;

        idym_test::validate(!(ex == value), "expected.object.eq.4");
        idym_test::validate(!(value == ex), "expected.object.eq.4");
        idym_test::validate(ex != value, "expected.object.eq.4");
        idym_test::validate(value != ex, "expected.object.eq.4");
    }
    {
        idym::expected<int, short> ex{idym::in_place, 123};
        long value = 124;

        idym_test::validate(!(ex == value), "expected.object.eq.4");
        idym_test::validate(!(value == ex), "expected.object.eq.4");
        idym_test::validate(ex != value, "expected.object.eq.4");
        idym_test::validate(value != ex, "expected.object.eq.4");
    }
}
void run_5_6() {
    {
        idym::expected<int, short> ex{idym::unexpect, 123};
        idym::unexpected<long> value{123};

        idym_test::validate(ex == value, "expected.object.eq.6");
        idym_test::validate(value == ex, "expected.object.eq.6");
        idym_test::validate(!(ex != value), "expected.object.eq.6");
        idym_test::validate(!(value != ex), "expected.object.eq.6");
    }
    {
        idym::expected<int, short> ex{idym::in_place, 123};
        idym::unexpected<long> value{123};

        idym_test::validate(!(ex == value), "expected.object.eq.6");
        idym_test::validate(!(value == ex), "expected.object.eq.6");
        idym_test::validate(ex != value, "expected.object.eq.6");
        idym_test::validate(value != ex, "expected.object.eq.6");
    }
    {
        idym::expected<int, short> ex{idym::unexpect, 123};
        idym::unexpected<long> value{124};

        idym_test::validate(!(ex == value), "expected.object.eq.6");
        idym_test::validate(!(value == ex), "expected.object.eq.6");
        idym_test::validate(ex != value, "expected.object.eq.6");
        idym_test::validate(value != ex, "expected.object.eq.6");
    }
}
}

/*
 * most of void is shared with the primary template, so
 * some cases are omitted for brevity(CBA)
 */

// [expected.void.cons]
namespace expected_void_cons {
void run_1() {
    {
        idym::expected<void, int> ex{};
        idym_test::validate(ex.has_value(), "expected.void.cons.1");
    }
    {
        idym::expected<volatile void, int> ex{};
        idym_test::validate(ex.has_value(), "expected.void.cons.1");
    }
}
void run_2_6() {
    static_assert(ex_copy_cons_v<void, idym_test::copy_ctor>, "expected.void.cons.5");
    static_assert(!ex_copy_cons_v<void, idym_test::ncopy_ctor>, "expected.void.cons.5");

    static_assert(ex_trivial_copy_cons<void, int>, "expected.void.cons.6");
    static_assert(!ex_trivial_copy_cons<void, idym_test::copy_ctor>, "expected.void.cons.6");

    {
        idym::expected<void, idym_test::copy_ctor> ex1{};
        auto ex2 = ex1;

        idym_test::validate(ex1.has_value(), "expected.void.cons.2");
        idym_test::validate(ex1.has_value() == ex2.has_value(), "expected.void.cons.2");
    }
    {
        idym::expected<void, idym_test::copy_ctor> ex1{idym::unexpect};
        auto ex2 = ex1;

        idym_test::validate(!ex1.has_value(), "expected.void.cons.2");
        idym_test::validate(ex1.has_value() == ex2.has_value(), "expected.void.cons.2");
        idym_test::validate(ex2.error().value == 1, "expected.void.cons.2");
    }
    {
        idym::expected<void, idym_test::copy_ctor_throws> ex1{idym::unexpect};
        IDYM_VALIDATE_EXCEPTION("expected.void.cons.9", idym::expected<void, idym_test::copy_ctor_throws>{ex1});
    }
}
void run_7_11() {
    static_assert(ex_move_cons_v<void, idym_test::move_ctor>, "expected.void.cons.7");
    static_assert(!ex_move_cons_v<void, idym_test::nmove_ctor>, "expected.void.cons.7");

    static_assert(ex_trivial_move_cons_v<void, int>, "expected.void.cons.11");
    static_assert(!ex_trivial_move_cons_v<void, idym_test::move_ctor>, "expected.void.cons.11");

    static_assert(ex_nothrow_move_cons_v<void, int>, "expected.void.cons.7");
    static_assert(!ex_nothrow_move_cons_v<void, idym_test::move_ctor_throws>, "expected.void.cons.7");

    {
        idym::expected<void, idym_test::move_ctor> ex1{};
        auto ex2 = std::move(ex1);

        idym_test::validate(ex1.has_value(), "expected.void.cons.8");
        idym_test::validate(ex1.has_value() == ex2.has_value(), "expected.void.cons.8");
    }
    {
        idym::expected<void, idym_test::move_ctor> ex1{idym::unexpect};
        auto ex2 = std::move(ex1);

        idym_test::validate(!ex1.has_value(), "expected.void.cons.8");
        idym_test::validate(ex1.has_value() == ex2.has_value(), "expected.void.cons.8");
        idym_test::validate(ex2.error().value == 1, "expected.void.cons.8");
    }
    {
        idym::expected<void, idym_test::move_ctor_throws> ex1{idym::unexpect};
        IDYM_VALIDATE_EXCEPTION("expected.void.cons.10", idym::expected<void, idym_test::move_ctor_throws>{std::move(ex1)});
    }
}
void run_12_16() {
    static_assert(ex_cons_v<void, double, idym::expected<const void, float>&&>, "expected.void.cons.13");
    static_assert(!ex_cons_v<void, double, idym::expected<const void, void*>&&>, "expected.void.cons.13");

    static_assert(ex_cons_v<void, double, const idym::expected<const void, float>&>, "expected.void.cons.13");
    static_assert(!ex_cons_v<void, double, const idym::expected<const void, void*>&>, "expected.void.cons.13");

    {
        const idym::expected<void, int> ex1{};
        const idym::expected<const void, int> ex2{ex1};

        idym_test::validate(ex1.has_value(), "expected.void.cons.15");
        idym_test::validate(ex1.has_value() == ex2.has_value(), "expected.void.cons.15");
    }
    {
        const idym::expected<void, int> ex1{idym::unexpect, 123};
        const idym::expected<const volatile void, int_constructible> ex2{ex1};

        idym_test::validate(!ex1.has_value(), "expected.void.cons.15");
        idym_test::validate(ex1.has_value() == ex2.has_value(), "expected.void.cons.15");
        idym_test::validate(ex2.error().value == 124, "expected.void.cons.14");
        IDYM_VALIDATE_EXCEPTION("expected.void.cons.16", idym::expected<volatile void, int_constructible_throws>{ex1});
    }
    {
        idym::expected<void, int> ex1{};
        const idym::expected<const void, int> ex2{std::move(ex1)};

        idym_test::validate(ex1.has_value(), "expected.void.cons.15");
        idym_test::validate(ex1.has_value() == ex2.has_value(), "expected.void.cons.15");
    }
    {
        idym::expected<void, int> ex1{idym::unexpect, 123};
        const idym::expected<const volatile void, int_constructible> ex2{std::move(ex1)};

        idym_test::validate(!ex1.has_value(), "expected.void.cons.15");
        idym_test::validate(ex1.has_value() == ex2.has_value(), "expected.void.cons.15");
        idym_test::validate(ex2.error().value == 125, "expected.void.cons.14");
        IDYM_VALIDATE_EXCEPTION("expected.void.cons.16", idym::expected<volatile void, int_constructible_throws>{std::move(ex1)});
    }
}
void run_17_21() {
    static_assert(ex_cons_v<void, int, idym::unexpected<int>>, "expected.void.cons.18");
    static_assert(ex_cons_v<void, int, idym::unexpected<double>>, "expected.void.cons.18");
    static_assert(!ex_cons_v<void, int, idym::unexpected<void*>>, "expected.void.cons.18");
    
    {
        const idym::unexpected<value_type> value{123};
        const idym::expected<void, value_type> ex{value};
        idym_test::validate(!ex.has_value(), "expected.void.cons.20");
        idym_test::validate(ex.error().value == 124, "expected.void.cons.19");
    }
    {
        idym::unexpected<value_type> value{123};
        const idym::expected<void, value_type> ex{std::move(value)};
        idym_test::validate(!ex.has_value(), "expected.void.cons.20");
        idym_test::validate(ex.error().value == 125, "expected.void.cons.19");
    }
    IDYM_VALIDATE_EXCEPTION("expected.void.cons.21", idym::expected<void, value_type>{idym::unexpected<void*>{nullptr}});
}
void run_22_30() {
    static_assert(ex_cons_v<void, il_constructible, idym::unexpect_t, int, void*>, "expected.void.cons.23");
    static_assert(!ex_cons_v<void, il_constructible, idym::unexpect_t, int, int, void*>, "expected.void.cons.23");
    static_assert(ex_cons_v<void, il_constructible, idym::unexpect_t, std::initializer_list<int>, void*>, "expected.void.cons.27");
    static_assert(!ex_cons_v<void, il_constructible, idym::unexpect_t, std::initializer_list<int>, void*, void*>, "expected.void.cons.27");
    
    {
        const idym::expected<void, il_constructible> ex{idym::unexpect, 1, nullptr};
        idym_test::validate(!ex.has_value(), "expected.void.cons.25");
        idym_test::validate(ex.error().value == 123, "expected.void.cons.24");
    }
    {
        const idym::expected<void, il_constructible> ex{idym::unexpect, {1}, nullptr};
        idym_test::validate(!ex.has_value(), "expected.void.cons.29");
        idym_test::validate(ex.error().value == 124, "expected.void.cons.28");
    }
    IDYM_VALIDATE_EXCEPTION("expected.void.cons.26", idym::expected<void, il_constructible>{idym::unexpect, nullptr, 1});
    IDYM_VALIDATE_EXCEPTION("expected.void.cons.30", idym::expected<void, il_constructible>{idym::unexpect, {static_cast<void*>(nullptr)}, 1});
}
}

// [expected.void.dtor]
namespace expected_void_dtor {
void run_1_2() {
    static_assert(ex_trivial_dtor_v<void, int>, "expected.void.2");
    static_assert(!ex_trivial_dtor_v<void, destructible>, "expected.void.2");
    
    {
        bool destroyed = false;
        idym::expected<const void, destructible>{idym::unexpect, &destroyed};
        idym_test::validate(destroyed, "expected.void.dtor.1");
    }
    {
        idym::expected<void, destructible>{};
    }
}
}

// [expected.void.assign]
namespace expected_void_assign {
template<bool Rvalue>
void validate_assignment(const char* clause_str, int same_member, int cross_member) {
    {
        idym::expected<void, value_type2<true, true>> ex1{};
        idym::expected<void, value_type2<true, true>> ex2{};
        
        ex2 = static_cast<fwd_assignment_t<decltype(ex1), Rvalue>>(ex1);
        idym_test::validate(ex1.has_value(), clause_str);
        idym_test::validate(ex2.has_value(), clause_str);
    }
    {
        idym::expected<void, value_type2<true, true>> ex1{idym::unexpect};
        idym::expected<void, value_type2<true, true>> ex2{};
        
        ex2 = static_cast<fwd_assignment_t<decltype(ex1), Rvalue>>(ex1);
        idym_test::validate(!ex1.has_value(), clause_str);
        idym_test::validate(!ex2.has_value(), clause_str);
        idym_test::validate(ex2.error().value == cross_member, clause_str);
    }
    {
        idym::expected<void, value_type2<true, true>> ex1{};
        idym::expected<void, value_type2<true, true>> ex2{idym::unexpect};
        
        ex2 = static_cast<fwd_assignment_t<decltype(ex1), Rvalue>>(ex1);
        idym_test::validate(ex1.has_value(), clause_str);
        idym_test::validate(ex2.has_value(), clause_str);
    }
    {
        idym::expected<void, value_type2<true, true>> ex1{idym::unexpect};
        idym::expected<void, value_type2<true, true>> ex2{idym::unexpect};
        
        ex2 = static_cast<fwd_assignment_t<decltype(ex1), Rvalue>>(ex1);
        idym_test::validate(!ex1.has_value(), clause_str);
        idym_test::validate(!ex2.has_value(), clause_str);
        idym_test::validate(ex2.error().value == same_member, clause_str);
    }
}

void run_1_7() {
    validate_assignment<false>("expected.void.assign.1", 125, 123);
    validate_assignment<true>("expected.void.assign.5", 126, 124);
}
void run_8_11() {
    {
        idym::expected<void, int_ctor> ex{};
        ex = idym::unexpected<int>{123};
        
        idym_test::validate(!ex.has_value(), "expected.void.assign.10");
        idym_test::validate(ex.error().value == 123, "expected.void.assign.10");
    }
    {
        idym::expected<void, int_ctor> ex{idym::unexpect};
        ex = idym::unexpected<int>{123};
        
        idym_test::validate(!ex.has_value(), "expected.void.assign.10");
        idym_test::validate(ex.error().value == 123, "expected.object.assign.10");
    }
}
void run_12() {
    {
        bool destroyed = false;
        idym::expected<const void, destructible> ex{idym::unexpect, &destroyed};
        
        ex.emplace();
        idym_test::validate(destroyed, "expected.void.assign.12");
        idym_test::validate(ex.has_value(), "expected.void.assign.12");
        
        ex.emplace();
        idym_test::validate(ex.has_value(), "expected.void.assign.12");
    }
}
}

// [expected.void.swap]
namespace expected_void_swap {
struct swap_throws {};

void swap(swap_throws&, swap_throws&) noexcept(false) {}

void run_1_5() {
    static_assert(idym::is_nothrow_swappable<idym::expected<void, int>>::value, "expected.void.swap.4");
    static_assert(!idym::is_nothrow_swappable<idym::expected<void, swap_throws>>::value, "expected.void.swap.4");

    {
        idym::expected<void, int> ex1{};
        idym::expected<void, int> ex2{};
        
        swap(ex1, ex2);
        idym_test::validate(ex1.has_value(), "expected.void.swap.2");
        idym_test::validate(ex2.has_value(), "expected.void.swap.2");
        
        ex1.swap(ex2);
        idym_test::validate(ex1.has_value(), "expected.void.swap.2");
        idym_test::validate(ex2.has_value(), "expected.void.swap.2");
    }
    {
        idym::expected<void, int> ex1{idym::unexpect, 123};
        idym::expected<void, int> ex2{};
        
        swap(ex1, ex2);
        idym_test::validate(ex1.has_value(), "expected.void.swap.2");
        idym_test::validate(!ex2.has_value(), "expected.void.swap.2");
        idym_test::validate(ex2.error() == 123, "expected.void.swap.2");
        
        ex1.swap(ex2);
        idym_test::validate(!ex1.has_value(), "expected.void.swap.2");
        idym_test::validate(ex2.has_value(), "expected.void.swap.2");
        idym_test::validate(ex1.error() == 123, "expected.void.swap.2");
    }
    {
        idym::expected<void, int> ex1{};
        idym::expected<void, int> ex2{idym::unexpect, 124};
        
        swap(ex1, ex2);
        idym_test::validate(!ex1.has_value(), "expected.void.swap.2");
        idym_test::validate(ex2.has_value(), "expected.void.swap.2");
        idym_test::validate(ex1.error() == 124, "expected.void.swap.2");
        
        ex1.swap(ex2);
        idym_test::validate(ex1.has_value(), "expected.void.swap.2");
        idym_test::validate(!ex2.has_value(), "expected.void.swap.2");
        idym_test::validate(ex2.error() == 124, "expected.void.swap.2");
    }
    {
        idym::expected<void, int> ex1{idym::unexpect, 123};
        idym::expected<void, int> ex2{idym::unexpect, 124};
        
        swap(ex1, ex2);
        idym_test::validate(!ex1.has_value(), "expected.void.swap.2");
        idym_test::validate(!ex2.has_value(), "expected.void.swap.2");
        idym_test::validate(ex1.error() == 124, "expected.void.swap.2");
        idym_test::validate(ex2.error() == 123, "expected.void.swap.2");
        
        ex1.swap(ex2);
        idym_test::validate(!ex1.has_value(), "expected.void.swap.2");
        idym_test::validate(!ex2.has_value(), "expected.void.swap.2");
        idym_test::validate(ex1.error() == 123, "expected.void.swap.2");
        idym_test::validate(ex2.error() == 124, "expected.void.swap.2");
    }
}
}

// [expected.void.obs]
namespace expected_void_obs {
void run_1() {
    {
        idym::expected<void, int> ex{};
        idym_test::validate(ex.has_value(), "expected.void.obs.1");
        idym_test::validate(static_cast<bool>(ex), "expected.void.obs.1");
    }
    {
        idym::expected<void, int> ex{idym::unexpect};
        idym_test::validate(!ex.has_value(), "expected.void.obs.1");
        idym_test::validate(!static_cast<bool>(ex), "expected.void.obs.1");
    }
}
void run_2_6() {
    {
        idym::expected<void, int> ex;
        *ex;
        ex.value();
        std::move(ex).value();
        
        static_assert(ex_expr_ret_v<decltype(*ex), void>, "expected.void.obs.2");
        static_assert(ex_expr_ret_v<decltype(ex.value()), void>, "expected.void.obs.3");
        static_assert(ex_expr_ret_v<decltype(std::move(ex).value()), void>, "expected.void.obs.5");
    }
    {
        idym::expected<void, int> ex{idym::unexpect, 123};
    
        bool caught = false;
        try {
            ex.value();
        } catch (idym::bad_expected_access<int> v) {
            caught = v.error() == 123;
        }
        idym_test::validate(caught, "expected.void.obs.4");
    }
    {
        idym::expected<void, int> ex{idym::unexpect, 123};
    
        bool caught = false;
        try {
            std::move(ex).value();
        } catch (idym::bad_expected_access<int> v) {
            caught = v.error() == 123;
        }
        idym_test::validate(caught, "expected.void.obs.6");
    }
}
void run_7_10() {
    static_assert(ex_expr_ret_v<decltype(std::declval<idym::expected<void, int>&>().error()), int&>, "expected.object.obs.7");
    static_assert(ex_expr_ret_v<decltype(std::declval<const idym::expected<void, int>&>().error()), const int&>, "expected.object.obs.7");

    static_assert(ex_expr_ret_v<decltype(std::declval<idym::expected<void, int>&&>().error()), int&&>, "expected.object.obs.9");
    static_assert(ex_expr_ret_v<decltype(std::declval<const idym::expected<void, int>&&>().error()), const int&&>, "expected.object.obs.9");

    {
        idym::expected<void, int> ex{idym::unexpect, 123};
        idym_test::validate(ex.error() == 123, "expected.object.obs.8");
        idym_test::validate(std::move(ex).error() == 123, "expected.object.obs.10");
    }
}
void run_11_14() {
    {
        idym::expected<void, int> ex{idym::unexpect, 123};
        idym_test::validate(ex.error_or(124) == 123, "expected.void.obs.12");
        idym_test::validate(std::move(ex).error_or(124) == 123, "expected.void.obs.14");
    }
    {
        idym::expected<void, int> ex{};
        idym_test::validate(ex.error_or(124) == 124, "expected.void.obs.12");
        idym_test::validate(std::move(ex).error_or(124) == 124, "expected.void.obs.14");
    }
}
}

// [expected.void.monadic]
namespace expected_void_monadic {
idym::expected<short, short> consume_value() {
    return idym::expected<short, short>{idym::in_place, 1};
}
idym::expected<void, long> consume_unex(short value) {
    return idym::expected<void, long>{idym::unexpect, value + 2};
}

short consume_value2() {
    return 3;
}
long consume_unex2(int value) {
    return value + 4;
}

void consume_void_value() {}

void run_1_8() {
    {
        idym::expected<void, short> ex{};

        const auto ret1 = ex.and_then(consume_value);
        idym_test::validate(ret1.has_value(), "expected.void.monadic.4");
        idym_test::validate(*ret1 == 1, "expected.void.monadic.4");

        const auto ret2 = std::move(ex).and_then(consume_value);
        idym_test::validate(ret2.has_value(), "expected.void.monadic.8");
        idym_test::validate(*ret2 == 1, "expected.void.monadic.8");

        static_assert(std::is_same<idym::remove_cvref_t<decltype(ret1)>, idym::expected<short, short>>::value, "expected.void.monadic.4");
        static_assert(std::is_same<idym::remove_cvref_t<decltype(ret2)>, idym::expected<short, short>>::value, "expected.void.monadic.8");
    }
    {
        idym::expected<void, short> ex{idym::unexpect, 123};

        const auto ret1 = ex.and_then(consume_value);
        idym_test::validate(!ret1.has_value(), "expected.void.monadic.4");
        idym_test::validate(ret1.error() == 123, "expected.void.monadic.4");

        const auto ret2 = std::move(ex).and_then(consume_value);
        idym_test::validate(!ret2.has_value(), "expected.void.monadic.8");
        idym_test::validate(ret2.error() == 123, "expected.void.monadic.8");

        static_assert(std::is_same<idym::remove_cvref_t<decltype(ret1)>, idym::expected<short, short>>::value, "expected.void.monadic.4");
        static_assert(std::is_same<idym::remove_cvref_t<decltype(ret2)>, idym::expected<short, short>>::value, "expected.void.monadic.8");
    }
}
void run_9_14() {
    {
        idym::expected<void, short> ex{};

        const auto ret1 = ex.or_else(consume_unex);
        idym_test::validate(ret1.has_value(), "expected.void.monadic.11");

        const auto ret2 = std::move(ex).or_else(consume_unex);
        idym_test::validate(ret2.has_value(), "expected.void.monadic.14");

        static_assert(std::is_same<idym::remove_cvref_t<decltype(ret1)>, idym::expected<void, long>>::value, "expected.void.monadic.11");
        static_assert(std::is_same<idym::remove_cvref_t<decltype(ret2)>, idym::expected<void, long>>::value, "expected.void.monadic.14");
    }
    {
        idym::expected<void, short> ex{idym::unexpect, 123};

        const auto ret1 = ex.or_else(consume_unex);
        idym_test::validate(!ret1.has_value(), "expected.void.monadic.11");
        idym_test::validate(ret1.error() == 125, "expected.void.monadic.11");

        const auto ret2 = std::move(ex).or_else(consume_unex);
        idym_test::validate(!ret2.has_value(), "expected.void.monadic.14");
        idym_test::validate(ret2.error() == 125, "expected.void.monadic.14");

        static_assert(std::is_same<idym::remove_cvref_t<decltype(ret1)>, idym::expected<void, long>>::value, "expected.void.monadic.11");
        static_assert(std::is_same<idym::remove_cvref_t<decltype(ret2)>, idym::expected<void, long>>::value, "expected.void.monadic.14");
    }
}
void run_15_22() {
    {
        idym::expected<void, short> ex{};

        const auto ret1 = ex.transform(consume_value2);
        idym_test::validate(ret1.has_value(), "expected.void.monadic.18");
        idym_test::validate(*ret1 == 3, "expected.void.monadic.18");

        const auto ret2 = std::move(ex).transform(consume_value2);
        idym_test::validate(ret2.has_value(), "expected.void.monadic.22");
        idym_test::validate(*ret2 == 3, "expected.void.monadic.22");

        static_assert(std::is_same<idym::remove_cvref_t<decltype(ret1)>, idym::expected<short, short>>::value, "expected.void.monadic.18");
        static_assert(std::is_same<idym::remove_cvref_t<decltype(ret2)>, idym::expected<short, short>>::value, "expected.void.monadic.22");
    }
    {
        idym::expected<void, short> ex{idym::unexpect, 123};

        const auto ret1 = ex.transform(consume_value2);
        idym_test::validate(!ret1.has_value(), "expected.void.monadic.18");
        idym_test::validate(ret1.error() == 123, "expected.void.monadic.18");

        const auto ret2 = std::move(ex).transform(consume_value2);
        idym_test::validate(!ret1.has_value(), "expected.void.monadic.22");
        idym_test::validate(ret1.error() == 123, "expected.void.monadic.22");

        static_assert(std::is_same<idym::remove_cvref_t<decltype(ret1)>, idym::expected<short, short>>::value, "expected.void.monadic.18");
        static_assert(std::is_same<idym::remove_cvref_t<decltype(ret2)>, idym::expected<short, short>>::value, "expected.void.monadic.22");
    }
    {
        idym::expected<void, short> ex{};

        const auto ret1 = ex.transform(consume_void_value);
        idym_test::validate(ret1.has_value(), "expected.void.monadic.18");
        
        const auto ret2 = std::move(ex).transform(consume_void_value);
        idym_test::validate(ret2.has_value(), "expected.void.monadic.22");
        
        static_assert(std::is_same<idym::remove_cvref_t<decltype(ret1)>, idym::expected<void, short>>::value, "expected.void.monadic.18");
        static_assert(std::is_same<idym::remove_cvref_t<decltype(ret2)>, idym::expected<void, short>>::value, "expected.void.monadic.22");
    }
    {
        idym::expected<void, short> ex{idym::unexpect, 123};

        const auto ret1 = ex.transform(consume_void_value);
        idym_test::validate(!ret1.has_value(), "expected.void.monadic.18");
        idym_test::validate(ret1.error() == 123, "expected.void.monadic.18");

        const auto ret2 = std::move(ex).transform(consume_void_value);
        idym_test::validate(!ret1.has_value(), "expected.void.monadic.22");
        idym_test::validate(ret1.error() == 123, "expected.void.monadic.22");

        static_assert(std::is_same<idym::remove_cvref_t<decltype(ret1)>, idym::expected<void, short>>::value, "expected.void.monadic.18");
        static_assert(std::is_same<idym::remove_cvref_t<decltype(ret2)>, idym::expected<void, short>>::value, "expected.void.monadic.22");
    }
}
void run_23_28() {
    {
        idym::expected<void, short> ex{};

        const auto ret1 = ex.transform_error(consume_unex2);
        idym_test::validate(ret1.has_value(), "expected.void.monadic.25");

        const auto ret2 = std::move(ex).transform_error(consume_unex2);
        idym_test::validate(ret2.has_value(), "expected.void.monadic.28");

        static_assert(std::is_same<idym::remove_cvref_t<decltype(ret1)>, idym::expected<void, long>>::value, "expected.void.monadic.25");
        static_assert(std::is_same<idym::remove_cvref_t<decltype(ret2)>, idym::expected<void, long>>::value, "expected.void.monadic.28");
    }
    {
        idym::expected<void, short> ex{idym::unexpect, 123};

        const auto ret1 = ex.transform_error(consume_unex2);
        idym_test::validate(!ret1.has_value(), "expected.void.monadic.25");
        idym_test::validate(ret1.error() == 127, "expected.void.monadic.25");

        const auto ret2 = std::move(ex).transform_error(consume_unex2);
        idym_test::validate(!ret1.has_value(), "expected.void.monadic.28");
        idym_test::validate(ret1.error() == 127, "expected.void.monadic.28");

        static_assert(std::is_same<idym::remove_cvref_t<decltype(ret1)>, idym::expected<void, long>>::value, "expected.void.monadic.25");
        static_assert(std::is_same<idym::remove_cvref_t<decltype(ret2)>, idym::expected<void, long>>::value, "expected.void.monadic.28");
    }
}
}

// [expected.void.eq]
namespace expected_void_eq {
void run_1_2() {
    {
        idym::expected<void, short> ex1{};
        idym::expected<const void, char> ex2{};

        idym_test::validate(ex1 == ex2, "expected.void.eq.2");
        idym_test::validate(ex2 == ex2, "expected.void.eq.2");
        idym_test::validate(!(ex1 != ex2), "expected.void.eq.2");
        idym_test::validate(!(ex2 != ex2), "expected.void.eq.2");
    }
    {
        idym::expected<void, short> ex1{idym::unexpect, 123};
        idym::expected<const void, char> ex2{idym::unexpect, 123};

        idym_test::validate(ex1 == ex2, "expected.void.eq.2");
        idym_test::validate(ex2 == ex1, "expected.void.eq.2");
        idym_test::validate(!(ex1 != ex2), "expected.void.eq.2");
        idym_test::validate(!(ex2 != ex1), "expected.void.eq.2");
    }
    {
        idym::expected<void, short> ex1{idym::unexpect, 123};
        idym::expected<const void, char> ex2{idym::unexpect, 124};

        idym_test::validate(!(ex1 == ex2), "expected.void.eq.2");
        idym_test::validate(!(ex2 == ex1), "expected.void.eq.2");
        idym_test::validate(ex1 != ex2, "expected.void.eq.2");
        idym_test::validate(ex2 != ex1, "expected.void.eq.2");
    }
    {
        idym::expected<void, short> ex1{idym::unexpect, 123};
        idym::expected<const void, char> ex2{};

        idym_test::validate(!(ex1 == ex2), "expected.void.eq.2");
        idym_test::validate(!(ex2 == ex1), "expected.void.eq.2");
        idym_test::validate(ex1 != ex2, "expected.void.eq.2");
        idym_test::validate(ex2 != ex1, "expected.void.eq.2");
    }
}
void run_3_4() {
    {
        idym::expected<void, short> ex{idym::unexpect, 123};
        idym::unexpected<long> value{123};

        idym_test::validate(ex == value, "expected.void.eq.4");
        idym_test::validate(value == ex, "expected.void.eq.4");
        idym_test::validate(!(ex != value), "expected.void.eq.4");
        idym_test::validate(!(value != ex), "expected.void.eq.4");
    }
    {
        idym::expected<void, short> ex{};
        idym::unexpected<long> value{123};

        idym_test::validate(!(ex == value), "expected.void.eq.4");
        idym_test::validate(!(value == ex), "expected.void.eq.4");
        idym_test::validate(ex != value, "expected.void.eq.4");
        idym_test::validate(value != ex, "expected.void.eq.4");
    }
    {
        idym::expected<void, short> ex{idym::unexpect, 123};
        idym::unexpected<long> value{124};

        idym_test::validate(!(ex == value), "expected.void.eq.4");
        idym_test::validate(!(value == ex), "expected.void.eq.4");
        idym_test::validate(ex != value, "expected.void.eq.4");
        idym_test::validate(value != ex, "expected.void.eq.4");
    }
}
}

int main(int, char**) {
    expected_un_cons::run_1_9();
    expected_un_obs::run_1_2();
    expected_un_eq::run_1_2();
    
    expected_object_cons::run_1_5();
    expected_object_cons::run_6_10();
    expected_object_cons::run_11_16();
    expected_object_cons::run_17_22();
    expected_object_cons::run_23_31();
    expected_object_cons::run_32_43();
    
    expected_object_dtor::run_1_2();
    
    expected_object_assign::run_2_8();
    expected_object_assign::run_9_15();
    expected_object_assign::run_16_19();
    
    expected_object_swap::run_1_5();

    expected_object_obs::run_1_2();
    expected_object_obs::run_3_6();
    expected_object_obs::run_7();
    expected_object_obs::run_8_13();
    expected_object_obs::run_14_17();
    expected_object_obs::run_18_25();

    expected_object_monadic::run_1_8();
    expected_object_monadic::run_9_16();
    expected_object_monadic::run_17_24();
    expected_object_monadic::run_25_32();

    expected_object_eq::run_1_2();
    expected_object_eq::run_3_4();
    expected_object_eq::run_5_6();

    expected_void_cons::run_1();
    expected_void_cons::run_2_6();
    expected_void_cons::run_7_11();
    expected_void_cons::run_12_16();
    expected_void_cons::run_17_21();
    expected_void_cons::run_22_30();
    
    expected_void_dtor::run_1_2();
    
    expected_void_assign::run_1_7();
    expected_void_assign::run_8_11();
    expected_void_assign::run_12();
    
    expected_void_swap::run_1_5();
    
    expected_void_obs::run_1();
    expected_void_obs::run_2_6();
    expected_void_obs::run_7_10();
    expected_void_obs::run_11_14();
    
    expected_void_monadic::run_1_8();
    expected_void_monadic::run_9_14();
    expected_void_monadic::run_15_22();
    expected_void_monadic::run_23_28();
    
    expected_void_eq::run_1_2();
    expected_void_eq::run_3_4();

    return 0;
}
