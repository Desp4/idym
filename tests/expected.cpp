#include <idym/expected.hpp>

#include "idym_test.hpp"

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
    struct value_type {
        value_type(int val) : value{val} {}
        value_type(void*) { throw idym_test::test_exception{}; }
        value_type(const value_type& other) : value{other.value + 1} {}
        value_type(value_type&& other) : value{other.value + 2} {}
        
        int value = 0;
    };

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
    struct il_constructible {
        il_constructible() = delete;
        il_constructible(int, void*) : value{123} {}
        il_constructible(std::initializer_list<int>, void*) : value{124} {}

        il_constructible(void*, int) { throw idym_test::test_exception{}; }
        il_constructible(std::initializer_list<void*>, int) { throw idym_test::test_exception{}; }
        
        int value;
    };
    
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
    static_assert(!ex_cons_v<int, il_constructible, idym::unexpect_t, std::initializer_list<int>, void*, void*>, "expected.object.cons.4");
    
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
template<typename T, typename E>
constexpr bool ex_trivial_dtor_v = std::is_trivially_destructible<idym::expected<T, E>>::value;

void run_1_2() {
    struct destructible {
        destructible(bool* fl) : flag{fl} {}
        ~destructible() { *flag = true; }
        
        bool* flag;
    };
    
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
template<bool Nothrow_Move, bool Noexcept_Copy = false>
struct value_type {
    value_type() = default;
    value_type(const value_type&) noexcept(Noexcept_Copy) : value{123} {}
    value_type(value_type&&) noexcept(Nothrow_Move) : value{124} {}
    
    value_type& operator=(const value_type&) noexcept(Noexcept_Copy) { value = 125; return *this; }
    value_type& operator=(value_type&&) noexcept(Nothrow_Move) { value = 126; return *this; }
    
    int value = 0;
};

template<typename T, bool Rvalue>
using fwd_assignment_t = std::conditional_t<Rvalue, T&&, T&>;

template<bool Rvalue, bool Nothrow_Move>
void validate_assignment(const char* clause_str, int same_member, int cross_member) {
    {
        idym::expected<value_type<Nothrow_Move>, value_type<true>> ex1{idym::in_place};
        idym::expected<value_type<Nothrow_Move>, value_type<true>> ex2{idym::in_place};
        
        ex2 = static_cast<fwd_assignment_t<decltype(ex1), Rvalue>>(ex1);
        idym_test::validate(ex2.has_value() == ex1.has_value(), clause_str);
        idym_test::validate(ex2->value == same_member, clause_str);
    }
    {
        idym::expected<value_type<true>, value_type<Nothrow_Move>> ex1{idym::unexpect};
        idym::expected<value_type<true>, value_type<Nothrow_Move>> ex2{idym::unexpect};
        
        ex2 = static_cast<fwd_assignment_t<decltype(ex1), Rvalue>>(ex1);
        idym_test::validate(ex2.has_value() == ex1.has_value(), clause_str);
        idym_test::validate(ex2.error().value == same_member, clause_str);
    }
    {
        idym::expected<value_type<Nothrow_Move>, value_type<true>> ex1{idym::unexpect};
        idym::expected<value_type<Nothrow_Move>, value_type<true>> ex2{idym::in_place};
        
        ex2 = static_cast<fwd_assignment_t<decltype(ex1), Rvalue>>(ex1);
        idym_test::validate(ex2.has_value() == ex1.has_value(), clause_str);
        idym_test::validate(ex2.error().value == cross_member, clause_str);
    }
    {
        idym::expected<value_type<true>, value_type<Nothrow_Move>> ex1{idym::in_place};
        idym::expected<value_type<true>, value_type<Nothrow_Move>> ex2{idym::unexpect};
        
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
        using nothrow_value_type = value_type<true, true>;
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
    struct int_ctor {
        int_ctor() = default;
        int_ctor(int v) : value{v} {}
        
        int value = 0;
    };

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

    return 0;
}
