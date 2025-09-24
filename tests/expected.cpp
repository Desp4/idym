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
}

int main(int, char**) {
    expected_un_cons::run_1_9();
    expected_un_obs::run_1_2();
    expected_un_eq::run_1_2();
    
    expected_object_cons::run_1_5();
    expected_object_cons::run_6_10();
    expected_object_cons::run_11_16();

    return 0;
}
