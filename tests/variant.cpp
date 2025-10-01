#include <idym/variant.hpp>

#include "idym_test.hpp"

#define IDYM_VALIDATE_BAD_ACCESS(STR, ...) IDYM_VALIDATE_EXCEPTION_GENERIC(STR, idym::bad_variant_access, __VA_ARGS__)

struct valueless_helper {
    valueless_helper() = default;
    valueless_helper(const valueless_helper&) = default;
    valueless_helper(valueless_helper&&) { throw idym_test::test_exception{}; }
    valueless_helper& operator=(const valueless_helper&) = default;
    valueless_helper& operator=(valueless_helper&&) { throw idym_test::test_exception{}; return *this; }
    
    friend bool operator==(valueless_helper, valueless_helper) { return true; }
    friend bool operator!=(valueless_helper, valueless_helper) { return true; }
    friend bool operator<(valueless_helper, valueless_helper) { return true; }
    friend bool operator>(valueless_helper, valueless_helper) { return true; }
    friend bool operator<=(valueless_helper, valueless_helper) { return true; }
    friend bool operator>=(valueless_helper, valueless_helper) { return true; }
};
template<typename... Ts>
using valueless_var_t = idym::variant<valueless_helper, int, Ts...>;

template<typename... Ts>
valueless_var_t<Ts...> make_valueless() {
    valueless_var_t<Ts...> v1;
    valueless_var_t<Ts...> v2{idym::in_place_index<1>, 0};
    try { v2 = std::move(v1); } catch (idym_test::test_exception) {}
    return v2;
}

// [variant.ctor]
namespace variant_ctor {

void run_1_6() {
    static_assert(!std::is_default_constructible<idym::variant<idym_test::ndef_ctor, int>>::value, "variant.ctor.1");
    static_assert(std::is_default_constructible<idym::variant<int, idym_test::ndef_ctor>>::value, "variant.ctor.1");
    
    static_assert(!std::is_nothrow_default_constructible<idym::variant<idym_test::def_ctor_throws>>::value, "variant.ctor.6");
    static_assert(std::is_nothrow_default_constructible<idym::variant<int>>::value, "variant.ctor.6");
    
    {
        idym::variant<idym_test::def_ctor> v;
        idym_test::validate(!v.valueless_by_exception(), "variant.ctor.4");
        idym_test::validate(v.index() == 0, "variant.ctor.4");
        idym_test::validate(idym::get<0>(v).value == 1337, "variant.ctor.3");
    }

    IDYM_VALIDATE_EXCEPTION("variant.ctor.5", idym::variant<idym_test::def_ctor_throws, int>{});
}
void run_7_9() {
    static_assert(std::is_copy_constructible<idym::variant<int>>::value, "variant.ctor.9");
    static_assert(std::is_trivially_copy_constructible<idym::variant<int>>::value, "variant.ctor.9");

    static_assert(std::is_copy_constructible<idym::variant<int, idym_test::copy_ctor>>::value, "variant.ctor.9");
    static_assert(!std::is_trivially_copy_constructible<idym::variant<int, idym_test::copy_ctor>>::value, "variant.ctor.9");

    static_assert(!std::is_copy_constructible<idym::variant<int, idym_test::ncopy_ctor>>::value, "variant.ctor.9");
    
    {
        idym::variant<idym_test::copy_ctor, int> v1;
        idym::get<0>(v1).value = 1337;
        
        idym::variant<idym_test::copy_ctor, int> v2{v1};
        idym_test::validate(v1.index() == v2.index(), "variant.ctor.7");
        idym_test::validate(idym::get<0>(v2).value == 1338, "variant.ctor.7");
    }
    {
        auto v1 = make_valueless<int>();
        valueless_var_t<int> v2{v1};
        idym_test::validate(v2.valueless_by_exception(), "variant.ctor.7");
    }
    {
        idym::variant<idym_test::copy_ctor_throws, int> v1;
        IDYM_VALIDATE_EXCEPTION("variant.ctor.8", idym::variant<idym_test::copy_ctor_throws, int>{v1});
    }
}
void run_10_13() {
    static_assert(std::is_move_constructible<idym::variant<int>>::value, "variant.ctor.10");
    static_assert(std::is_trivially_move_constructible<idym::variant<int>>::value, "variant.ctor.13");
    static_assert(std::is_nothrow_move_constructible<idym::variant<int>>::value, "variant.ctor.13");
    
    static_assert(std::is_move_constructible<idym::variant<int, idym_test::move_ctor>>::value, "variant.ctor.10");
    static_assert(!std::is_trivially_move_constructible<idym::variant<int, idym_test::move_ctor>>::value, "variant.ctor.13");
    static_assert(std::is_nothrow_move_constructible<idym::variant<int, idym_test::move_ctor>>::value, "variant.ctor.13");
    
    static_assert(std::is_move_constructible<idym::variant<int, idym_test::move_ctor_throws>>::value, "variant.ctor.10");
    static_assert(!std::is_trivially_move_constructible<idym::variant<int, idym_test::move_ctor_throws>>::value, "variant.ctor.13");
    static_assert(!std::is_nothrow_move_constructible<idym::variant<int, idym_test::move_ctor_throws>>::value, "variant.ctor.13");
    
    static_assert(!std::is_move_constructible<idym::variant<int, idym_test::nmove_ctor>>::value, "variant.ctor.10");
    
    {
        idym::variant<idym_test::move_ctor, int> v1;
        idym::get<0>(v1).value = 1337;
        
        idym::variant<idym_test::move_ctor, int> v2{std::move(v1)};
        idym_test::validate(v1.index() == v2.index(), "variant.ctor.11");
        idym_test::validate(idym::get<0>(v2).value == 1338, "variant.ctor.11");
    }
    {
        auto v1 = make_valueless<int>();
        valueless_var_t<int> v2{std::move(v1)};
        idym_test::validate(v2.valueless_by_exception(), "variant.ctor.11");
    }
    {
        idym::variant<idym_test::move_ctor_throws, int> v1;
        IDYM_VALIDATE_EXCEPTION("variant.ctor.12", idym::variant<idym_test::move_ctor_throws, int>{std::move(v1)});
    }
}
void run_14_19() {
    // bundling this and 20_29 with variant.ctor.30 through 38 - can't be arsed
    struct int_ctor {
        int_ctor(int v) noexcept : value{v} {}
        int value;
    };
    struct char_ctor {
        char_ctor(char v) : value{v} {}
        char value;
    };
    struct ptr_ctor {
        ptr_ctor(void*) { throw idym_test::test_exception{}; }
    };
    
    struct move_copy_ctor {
        move_copy_ctor(int init_value = 0) : value{init_value} {}
        move_copy_ctor(const move_copy_ctor& other) : state_flag{1}, value{other.value} {}
        move_copy_ctor(move_copy_ctor&& other) : state_flag{2}, value{other.value} {}
        
        int state_flag = 0;
        int value = 0;
    };
    
    static_assert(std::is_constructible<idym::variant<int_ctor, int, char_ctor>, char>::value, "variant.ctor.15");
    static_assert(!std::is_constructible<idym::variant<int_ctor, char_ctor>, char>::value, "variant.ctor.15");
    static_assert(!std::is_constructible<idym::variant<int_ctor, int_ctor>, int>::value, "variant.ctor.15");
    static_assert(std::is_constructible<idym::variant<int_ctor, int_ctor, ptr_ctor>, void*>::value, "variant.ctor.15");
    
    static_assert(std::is_nothrow_constructible<idym::variant<int, void*>, int>::value, "variant.ctor.19");
    static_assert(std::is_nothrow_constructible<idym::variant<int_ctor, ptr_ctor>, int>::value, "variant.ctor.19");
    static_assert(!std::is_nothrow_constructible<idym::variant<int_ctor, ptr_ctor>, void*>::value, "variant.ctor.19");

    {
        idym::variant<void*, move_copy_ctor> v{42};
        idym_test::validate(idym::holds_alternative<move_copy_ctor>(v), "variant.ctor.17");
        idym_test::validate(idym::get<1>(v).value == 42, "variant.ctor.16");
    }
    {
        move_copy_ctor value{42};
        idym::variant<void*, move_copy_ctor> v{std::move(value)};
        idym_test::validate(idym::holds_alternative<move_copy_ctor>(v), "variant.ctor.17");
        idym_test::validate(idym::get<1>(v).value == 42, "variant.ctor.17");
        idym_test::validate(idym::get<1>(v).state_flag == 2, "variant.ctor.17");
    }
    IDYM_VALIDATE_EXCEPTION("variant.ctor.18", idym::variant<ptr_ctor, int>{nullptr});
}
void run_20_29() {
    struct init_list_type {
        init_list_type(std::initializer_list<int> ints, int v) : ints_size{ints.size()}, value{v} {}
        
        std::size_t ints_size;
        int value;
    };
    struct init_list_type_throws {
        init_list_type_throws(std::initializer_list<int>) { throw idym_test::test_exception{}; }
    };
    struct throws {
        throws(int, int) { throw idym_test::test_exception{}; }
    };

    static_assert(!std::is_constructible<idym::variant<int, int>, idym::in_place_type_t<int>, int>::value, "variant.ctor.20.1");
    static_assert(std::is_constructible<idym::variant<int, void*>, idym::in_place_type_t<int>, int>::value, "variant.ctor.20.1");
    
    static_assert(!std::is_constructible<idym::variant<int, void*>, idym::in_place_type_t<int>, void*>::value, "variant.ctor.20.2");
    static_assert(std::is_constructible<idym::variant<int, void*>, idym::in_place_type_t<int>, int>::value, "variant.ctor.20.2");
    
    static_assert(!std::is_constructible<idym::variant<init_list_type, init_list_type>, idym::in_place_type_t<init_list_type>, std::initializer_list<int>, int>::value, "variant.ctor.25.1");
    static_assert(std::is_constructible<idym::variant<init_list_type, int>, idym::in_place_type_t<init_list_type>, std::initializer_list<int>, int>::value, "variant.ctor.25.1");
    
    static_assert(!std::is_constructible<idym::variant<init_list_type, int>, idym::in_place_type_t<init_list_type>, std::initializer_list<int>, void*>::value, "variant.ctor.25.2");
    static_assert(std::is_constructible<idym::variant<init_list_type, int>, idym::in_place_type_t<init_list_type>, std::initializer_list<int>, int>::value, "variant.ctor.25.2");
    
    {
        idym::variant<int, void*> v{idym::in_place_type<int>, 4};
        idym_test::validate(idym::holds_alternative<int>(v), "variant.ctor.22");
        idym_test::validate(idym::get<0>(v) == 4, "variant.ctor.21");
    }
    {
        idym::variant<int, init_list_type> v{idym::in_place_type<init_list_type>, {1, 2, 3}, 4};
        idym_test::validate(idym::holds_alternative<init_list_type>(v), "variant.ctor.27");
        idym_test::validate(idym::get<1>(v).ints_size == 3, "variant.ctor.26");
        idym_test::validate(idym::get<1>(v).value == 4, "variant.ctor.26");
    }
    IDYM_VALIDATE_EXCEPTION("variant.ctor.23", idym::variant<throws, int>{idym::in_place_type<throws>, 1, 2});
    IDYM_VALIDATE_EXCEPTION("variant.ctor.28", idym::variant<init_list_type_throws, int>{idym::in_place_type<init_list_type_throws>, {1, 2}});
}

}

// [variant.dtor]
namespace variant_dtor {

void run_1_2() {
    struct dtor_type {
        dtor_type(bool* flag) : dtor_flag{flag} {}
        ~dtor_type() {
            *dtor_flag = true;
        }
        
        bool* dtor_flag;
    };
    
    static_assert(std::is_trivially_destructible<idym::variant<int, int>>::value, "variant.dtor.2");
    static_assert(!std::is_trivially_destructible<idym::variant<int, dtor_type>>::value, "variant.dtor.2");
    
    {
        bool dtor_flag = false;
        {
            idym::variant<int, dtor_type> v{idym::in_place_type<dtor_type>, &dtor_flag};
        }
        idym_test::validate(dtor_flag, "variant.dtor.1");
    }
}

}

// [variant.assign]
namespace variant_assign {

template<int Id_Dummy, bool Noexcept = true>
struct assign_type {
    int state = 0;
    int* dtor_count;
    
    assign_type(int* count) : dtor_count{count} {}
    assign_type(const assign_type& other) {
        state = 1;
        dtor_count = other.dtor_count;
    }
    assign_type(assign_type&& other) noexcept(Noexcept) {
        state = 2;
        dtor_count = other.dtor_count;
    }
    ~assign_type() {
        ++(*dtor_count);
    }
    
    assign_type& operator=(const assign_type& other) {
        state = 3;
        dtor_count = other.dtor_count;
        return *this;
    }
    assign_type& operator=(assign_type&& other) noexcept(Noexcept) {
        state = 4;
        dtor_count = other.dtor_count;
        return *this;
    }
};

void run_1_5() {
    struct custom_copy {
        custom_copy(const custom_copy&) {}
        custom_copy& operator=(const custom_copy&) { return *this; }
    };

    struct ncopy_ctor {
        ncopy_ctor(const ncopy_ctor&) = delete;
        ncopy_ctor& operator=(const ncopy_ctor&) = default;
    };
    struct ncopy_ass {
        ncopy_ass(const ncopy_ass&) = default;
        ncopy_ass& operator=(const ncopy_ass&) = delete;
    };
    
    static_assert(std::is_copy_assignable<idym::variant<int, double>>::value, "variant.assign.5");
    static_assert(std::is_copy_assignable<idym::variant<int, custom_copy>>::value, "variant.assign.5");
    static_assert(!std::is_copy_assignable<idym::variant<int, ncopy_ctor>>::value, "variant.assign.5");
    static_assert(!std::is_copy_assignable<idym::variant<int, ncopy_ass>>::value, "variant.assign.5");
    
    static_assert(std::is_trivially_copy_assignable<idym::variant<int, double>>::value, "variant.assign.5");
    static_assert(!std::is_trivially_copy_assignable<idym::variant<int, custom_copy>>::value, "variant.assign.5");

    {
        int dtor_count = 0;
        auto v1 = make_valueless<assign_type<0>>();
        auto v2 = make_valueless<assign_type<0>>();
        valueless_var_t<assign_type<0>> v3{idym::in_place_type<assign_type<0>>, &dtor_count};
        
        v2 = v1;
        v3 = v1;
        idym_test::validate(v2.valueless_by_exception(), "variant.assign.2.1");
        idym_test::validate(v3.valueless_by_exception(), "variant.assign.2.2");
        idym_test::validate(dtor_count == 1, "variant.assign.2.2");
    }
    {
        int dtor_count = 0;
        idym::variant<assign_type<0>, int> v1{&dtor_count};
        idym::variant<assign_type<0>, int> v2{&dtor_count};
        
        auto& v2_ret = v2 = v1;
        idym_test::validate(&v2_ret == &v2, "variant.assign.4");
        
        idym_test::validate(v1.index() == v2.index(), "variant.assign.3");
        idym_test::validate(idym::get<0>(v2).state == 3, "variant.assign.2.3");
        idym_test::validate(dtor_count == 0, "variant.assign.2.3");
    }
    {
        int dtor_count1 = 0;
        int dtor_count2 = 0;
        int dtor_count3 = 0;
        idym::variant<assign_type<0, true>, assign_type<0, false>> v1{idym::in_place_type<assign_type<0, true>>, &dtor_count1};
        idym::variant<assign_type<0, true>, assign_type<0, false>> v2{idym::in_place_type<assign_type<0, true>>, &dtor_count2};
        idym::variant<assign_type<0, true>, assign_type<0, false>> v3{idym::in_place_type<assign_type<0, false>>, &dtor_count3};
        
        v1 = v3;
        idym_test::validate(v1.index() == v3.index(), "variant.assign.3");
        idym_test::validate(idym::get<1>(v1).state == 1, "variant.assign.2.4");
        
        v1 = v2;
        idym_test::validate(v1.index() == v2.index(), "variant.assign.3");
        idym_test::validate(idym::get<0>(v1).state == 2, "variant.assign.2.5");
        
        idym_test::validate(dtor_count1 == 1, "variant.assign.2.4");
        idym_test::validate(dtor_count2 == 1, "variant.assign.2.5");
        idym_test::validate(dtor_count3 == 1, "variant.assign.2.5");
    }
}
void run_6_10() {
    struct custom_move {
        custom_move(custom_move&&) noexcept {}
        custom_move& operator=(custom_move&&) noexcept { return *this; }
    };

    struct nmove_ctor {
        nmove_ctor(const nmove_ctor&) = delete;
        nmove_ctor& operator=(const nmove_ctor&) = default;
    };
    struct nmove_ass {
        nmove_ass(nmove_ass&&) = default;
        nmove_ass& operator=(nmove_ass&&) = delete;
    };
    
    struct move_throws {
        move_throws() = default;
        move_throws(move_throws&&) { throw idym_test::test_exception{}; }
        move_throws& operator=(move_throws&&) { throw idym_test::test_exception{}; return *this; }
    };
    
    static_assert(std::is_move_assignable<idym::variant<int, custom_move>>::value, "variant.assign.7");
    static_assert(std::is_move_assignable<idym::variant<int, double>>::value, "variant.assign.7");
    static_assert(!std::is_move_assignable<idym::variant<int, nmove_ctor>>::value, "variant.assign.7");
    static_assert(!std::is_move_assignable<idym::variant<int, nmove_ass>>::value, "variant.assign.7");
    
    static_assert(std::is_trivially_move_assignable<idym::variant<int, double>>::value, "variant.assign.10");
    static_assert(!std::is_trivially_move_assignable<idym::variant<int, custom_move>>::value, "variant.assign.10");
    
    static_assert(std::is_nothrow_move_assignable<idym::variant<int, double>>::value, "variant.assign.10");
    static_assert(std::is_nothrow_move_assignable<idym::variant<int, custom_move>>::value, "variant.assign.10");
    static_assert(!std::is_nothrow_move_assignable<idym::variant<int, move_throws>>::value, "variant.assign.10");

    {
        int dtor_count = 0;
        auto v1 = make_valueless<assign_type<0>>();
        auto v2 = make_valueless<assign_type<0>>();
        auto v3 = make_valueless<assign_type<0>>();
        valueless_var_t<assign_type<0>> v4{idym::in_place_type<assign_type<0>>, &dtor_count};
        
        v3 = std::move(v1);
        v4 = std::move(v2);
        idym_test::validate(v3.valueless_by_exception(), "variant.assign.8.1");
        idym_test::validate(v4.valueless_by_exception(), "variant.assign.8.2");
        idym_test::validate(dtor_count == 1, "variant.assign.8.2");
    }
    {
        int dtor_count = 0;
        idym::variant<assign_type<0>, int> v1{&dtor_count};
        idym::variant<assign_type<0>, int> v2{&dtor_count};
        
        auto& v2_ret = v2 = std::move(v1);
        idym_test::validate(&v2_ret == &v2, "variant.assign.9");
        
        idym_test::validate(idym::get<0>(v2).state == 4, "variant.assign.8.3");
        idym_test::validate(dtor_count == 0, "variant.assign.8.3");
    }
    {
        int dtor_count1 = 0;
        int dtor_count2 = 0;
        idym::variant<assign_type<0>, assign_type<1>> v1{idym::in_place_index<0>, &dtor_count1};
        idym::variant<assign_type<0>, assign_type<1>> v2{idym::in_place_index<1>, &dtor_count2};
        
        v1 = std::move(v2);
        idym_test::validate(idym::get<1>(v1).state == 2, "variant.assign.8.4");
        idym_test::validate(dtor_count1 == 1, "variant.assign.8.4");
        idym_test::validate(dtor_count2 == 0, "variant.assign.8.4");
    }
    {
        idym::variant<int, move_throws> v1{idym::in_place_index<0>};
        idym::variant<int, move_throws> v2{idym::in_place_index<1>};
        
        IDYM_VALIDATE_EXCEPTION("variant.assign.10.1", v1 = std::move(v2));
        idym_test::validate(v1.valueless_by_exception(), "variant.assign.10.1");
    }
    {
        idym::variant<int, move_throws> v1{idym::in_place_index<1>};
        idym::variant<int, move_throws> v2{idym::in_place_index<1>};
        
        IDYM_VALIDATE_EXCEPTION("variant.assign.10.2", v1 = std::move(v2));
        idym_test::validate(v1.index() == 1, "variant.assign.10.2");
    }
}
void run_11_16() {
    struct int_ctor {
        int_ctor(int v) noexcept : value{v} {}
        int value;
    };
    struct char_ctor {
        char_ctor(char v) : value{v} {}
        char value;
    };
    struct ptr_ctor {
        ptr_ctor(void*) {}
        ptr_ctor& operator=(const ptr_ctor&) { throw idym_test::test_exception{}; return *this; }
    };
    
    static_assert(std::is_assignable<idym::variant<int_ctor, int, char_ctor>, char>::value, "variant.assign.12.3");
    static_assert(!std::is_assignable<idym::variant<int_ctor, int_ctor>, int>::value, "variant.assign.12.3");
    static_assert(!std::is_assignable<idym::variant<int_ctor, char>, void*>::value, "variant.assign.12.2");
    
    static_assert(std::is_nothrow_assignable<idym::variant<int_ctor, ptr_ctor>, int>::value, "variant.assign.16.1");
    static_assert(!std::is_nothrow_assignable<idym::variant<int_ctor, ptr_ctor>, void*>::value, "variant.assign.16.1");
    
    {
        int dtor_count = 0;
        idym::variant<assign_type<0>, int> v1{&dtor_count};
        v1 = assign_type<0>{&dtor_count};
        
        idym_test::validate(dtor_count == 1, "variant.assign.13.1");
        idym_test::validate(v1.index() == 0, "variant.assign.14");
        idym_test::validate(idym::get<0>(v1).state == 4, "variant.assign.13.1");
    }
    {
        int dtor_count = 0;
        idym::variant<assign_type<0>, int> v1{idym::in_place_index<1>};
        v1 = assign_type<0>{&dtor_count};
        
        idym_test::validate(dtor_count == 1, "variant.assign.13.2");
        idym_test::validate(v1.index() == 0, "variant.assign.14");
        idym_test::validate(idym::get<0>(v1).state == 2, "variant.assign.13.2");
    }
    {
        idym::variant<ptr_ctor, int> v1{nullptr};
        IDYM_VALIDATE_EXCEPTION("variant.assign.16.1", v1 = ptr_ctor{nullptr});
        
        idym_test::validate(v1.index() == 0, "variant.assign.16.1");
    }
}

}

// [variant.mod]
namespace variant_mod {

void run_1_18() {
    struct init_list_type {
        init_list_type(std::initializer_list<int> ints, int v) : ints_size{ints.size()}, value{v} {}
        
        std::size_t ints_size;
        int value;
    };
    struct int_type {
        int_type(int v) : value{v} {}
        int value;
    };
    
    {
        idym::variant<int, init_list_type, int_type> v1{0};
        auto& value_ref = v1.emplace<1>({1, 2, 3}, 4);
        
        idym_test::validate(v1.index() == 1, "variant.mod.15");
        idym_test::validate(&value_ref == &idym::get<1>(v1), "variant.mod.16");
        idym_test::validate(idym::get<1>(v1).ints_size == 3, "variant.mod.14");
        idym_test::validate(idym::get<1>(v1).value == 4, "variant.mod.14");
    }
    {
        idym::variant<int, init_list_type, int_type> v1{0};
        auto& value_ref = v1.emplace<init_list_type>({1, 2, 3}, 4);
        
        idym_test::validate(v1.index() == 1, "variant.mod.4");
        idym_test::validate(&value_ref == &idym::get<1>(v1), "variant.mod.4");
        idym_test::validate(idym::get<1>(v1).ints_size == 3, "variant.mod.4");
        idym_test::validate(idym::get<1>(v1).value == 4, "variant.mod.4");
    }
    {
        idym::variant<int, init_list_type, int_type> v1{0};
        auto& value_ref = v1.emplace<2>(4);
        
        idym_test::validate(v1.index() == 2, "variant.mod.8");
        idym_test::validate(&value_ref == &idym::get<2>(v1), "variant.mod.9");
        idym_test::validate(idym::get<2>(v1).value == 4, "variant.mod.7");
    }
    {
        idym::variant<int, init_list_type, int_type> v1{0};
        auto& value_ref = v1.emplace<int_type>(4);
        
        idym_test::validate(v1.index() == 2, "variant.mod.2");
        idym_test::validate(&value_ref == &idym::get<2>(v1), "variant.mod.2");
        idym_test::validate(idym::get<2>(v1).value == 4, "variant.mod.2");
    }
}

}

// [variant.status]
namespace variant_status {

void run_1_3() {
    {
        const auto valueless = make_valueless();
        idym_test::validate(valueless.valueless_by_exception(), "variant.status.2");
        idym_test::validate(valueless.index() == idym::variant_npos, "variant.status.3");
    }
    {
        idym::variant<int, void*> v1{4};
        idym_test::validate(!v1.valueless_by_exception(), "variant.status.1");
        idym_test::validate(v1.index() == 0, "variant.status.3");
    }
}

}

// [variant.swap]
namespace variant_swap {

struct default_swappable {
    default_swappable(int v) : value{v} {}
    int value;
};

struct swappable {
    int swapped_status = 0;
};

void swap(swappable& lhs, swappable& rhs) noexcept(false) {
    lhs.swapped_status = 1;
    rhs.swapped_status = 2;
}

void run_1_5() {
    {
        auto v1 = make_valueless();
        auto v2 = make_valueless();
        v1.swap(v2);
        
        idym_test::validate(v1.valueless_by_exception(), "variant.swap.3.1");
        idym_test::validate(v2.valueless_by_exception(), "variant.swap.3.1");
    }
    {
        idym::variant<swappable, int> v1;
        idym::variant<swappable, int> v2;
        
        v1.swap(v2);
        idym_test::validate(idym::get<0>(v1).swapped_status == 1, "variant.swap.3.2");
        idym_test::validate(idym::get<0>(v2).swapped_status == 2, "variant.swap.3.2");

        // msvc 19.16 doesn't understand noexcept(false) in dependent contexts
#if _MSC_VER > 1916
        static_assert(!noexcept(v1.swap(v2)), "variant.swap.5");
#endif
    }
    {
        idym::variant<default_swappable, int> v1{idym::in_place_index<0>, 5};
        idym::variant<default_swappable, int> v2{idym::in_place_index<0>, 4};
        
        v1.swap(v2);
        idym_test::validate(idym::get<0>(v1).value == 4, "variant.swap.3.2");
        idym_test::validate(idym::get<0>(v2).value == 5, "variant.swap.3.2");
        
        static_assert(noexcept(v1.swap(v2)), "variant.swap.5");
    }
    {
        idym::variant<default_swappable, int> v1{idym::in_place_index<0>, 5};
        idym::variant<default_swappable, int> v2{idym::in_place_index<1>, 4};
        
        v1.swap(v2);
        idym_test::validate(v1.index() == 1, "variant.swap.3.3");
        idym_test::validate(v2.index() == 0, "variant.swap.3.3");
        idym_test::validate(idym::get<1>(v1) == 4, "variant.swap.3.3");
        idym_test::validate(idym::get<0>(v2).value == 5, "variant.swap.3.3");
    }
}

}

// [variant.helper]
namespace variant_helper {

static_assert(idym::variant_size_v<idym::variant<int, char, char>> == 3, "variant.helper.1");
static_assert(idym::variant_size_v<const idym::variant<int, char, char>> == 3, "variant.helper.2");

static_assert(std::is_same<char, idym::variant_alternative_t<1, idym::variant<int, char, char>>>::value, "variant.helper.3");
static_assert(std::is_same<int, idym::variant_alternative_t<0, const idym::variant<int, char, char>>>::value, "variant.helper.3");

}

// [variant.get]
namespace variant_get {

void run_1_2() {
    {
        idym::variant<int, char> v1{};
        idym_test::validate(idym::holds_alternative<int>(v1), "variant.get.2");
        idym_test::validate(!idym::holds_alternative<char>(v1), "variant.get.2");
    }
}
void run_3_9() {
    {
        idym::variant<int, char> v1{};
        const auto& cv1 = v1;
        
        idym::get<0>(v1);
        IDYM_VALIDATE_BAD_ACCESS("variant.get.7", idym::get<1>(v1));
        
        static_assert(std::is_same<decltype(idym::get<0>(v1)), int&>::value, "variant.get.7");
        static_assert(std::is_same<decltype(idym::get<0>(std::move(v1))), int&&>::value, "variant.get.7");
        static_assert(std::is_same<decltype(idym::get<0>(cv1)), const int&>::value, "variant.get.7");
        static_assert(std::is_same<decltype(idym::get<0>(std::move(cv1))), const int&&>::value, "variant.get.7");
    }
    {
        idym::variant<int, char> v1{};
        const auto& cv1 = v1;
        
        idym::get<int>(v1);
        IDYM_VALIDATE_BAD_ACCESS("variant.get.9", idym::get<char>(v1));
        
        static_assert(std::is_same<decltype(idym::get<int>(v1)), int&>::value, "variant.get.9");
        static_assert(std::is_same<decltype(idym::get<int>(std::move(v1))), int&&>::value, "variant.get.9");
        static_assert(std::is_same<decltype(idym::get<int>(cv1)), const int&>::value, "variant.get.9");
        static_assert(std::is_same<decltype(idym::get<int>(std::move(cv1))), const int&&>::value, "variant.get.9");
    }
}
void run_10_13() {
    {
        idym::variant<int, char> v1{};
        const auto& cv1 = v1;
        
        idym_test::validate(idym::get_if<0>(&v1), "variant.get.11");
        idym_test::validate(!idym::get_if<1>(&v1), "variant.get.11");
        
        static_assert(std::is_same<decltype(idym::get_if<0>(&v1)), int*>::value, "variant.get.11");
        static_assert(std::is_same<decltype(idym::get_if<0>(&cv1)), const int*>::value, "variant.get.11");
    }
    {
        idym::variant<int, char> v1{};
        const auto& cv1 = v1;
        
        idym_test::validate(idym::get_if<int>(&v1), "variant.get.13");
        idym_test::validate(!idym::get_if<char>(&v1), "variant.get.13");
        
        static_assert(std::is_same<decltype(idym::get_if<int>(&v1)), int*>::value, "variant.get.13");
        static_assert(std::is_same<decltype(idym::get_if<int>(&cv1)), const int*>::value, "variant.get.13");
    }
}

}

// [variant.relops]
namespace variant_relops {

struct comparable {
    comparable(int v) : value{v} {}
    int value = 0;
};
bool operator==(const comparable& lhs, const comparable& rhs) {
    return lhs.value == rhs.value;
}
bool operator!=(const comparable& lhs, const comparable& rhs) {
    return lhs.value != rhs.value;
}
bool operator<(const comparable& lhs, const comparable& rhs) {
    return lhs.value < rhs.value;
}
bool operator>(const comparable& lhs, const comparable& rhs) {
    return lhs.value > rhs.value;
}
bool operator<=(const comparable& lhs, const comparable& rhs) {
    return lhs.value <= rhs.value;
}
bool operator>=(const comparable& lhs, const comparable& rhs) {
    return lhs.value >= rhs.value;
}

void run_1_2() {
    {
        const auto lhs = make_valueless<comparable>();
        const auto rhs = make_valueless<comparable>();
        idym_test::validate(lhs == rhs, "variant.relops.2");
    }
    {
        const idym::variant<comparable, int> lhs{idym::in_place_index<0>, 1};
        const idym::variant<comparable, int> rhs{idym::in_place_index<1>, 1};
        idym_test::validate(!(lhs == rhs), "variant.relops.2");
    }
    {
        idym::variant<comparable, int> lhs{idym::in_place_index<0>, 1};
        idym::variant<comparable, int> rhs{idym::in_place_index<0>, 1};
        idym_test::validate(lhs == rhs, "variant.relops.2");
        
        idym::get<0>(rhs).value = 2;
        idym_test::validate(!(lhs == rhs), "variant.relops.2");
    }
}
void run_3_4() {
    {
        const auto lhs = make_valueless<comparable>();
        const auto rhs = make_valueless<comparable>();
        idym_test::validate(!(lhs != rhs), "variant.relops.4");
    }
    {
        const idym::variant<comparable, int> lhs{idym::in_place_index<0>, 1};
        const idym::variant<comparable, int> rhs{idym::in_place_index<1>, 1};
        idym_test::validate(lhs != rhs, "variant.relops.4");
    }
    {
        idym::variant<comparable, int> lhs{idym::in_place_index<0>, 1};
        idym::variant<comparable, int> rhs{idym::in_place_index<0>, 1};
        idym_test::validate(!(lhs != rhs), "variant.relops.4");
        
        idym::get<0>(rhs).value = 2;
        idym_test::validate(lhs != rhs, "variant.relops.4");
    }
}
void run_5_6() {
    {
        const auto lhs = make_valueless<comparable>();
        const valueless_var_t<comparable> rhs{idym::in_place_index<2>, 1};
        idym_test::validate(lhs < rhs, "variant.relops.6");
    }
    {
        const valueless_var_t<comparable> lhs{idym::in_place_index<2>, 1};
        const auto rhs = make_valueless<comparable>();
        idym_test::validate(!(lhs < rhs), "variant.relops.6");
    }
    {
        const idym::variant<comparable, int> lhs{idym::in_place_index<0>, 1};
        const idym::variant<comparable, int> rhs{idym::in_place_index<1>, 1};
        idym_test::validate(lhs < rhs, "variant.relops.6");
    }
    {
        const idym::variant<comparable, int> lhs{idym::in_place_index<1>, 1};
        const idym::variant<comparable, int> rhs{idym::in_place_index<0>, 1};
        idym_test::validate(!(lhs < rhs), "variant.relops.6");
    }
    {
        idym::variant<comparable, int> lhs{idym::in_place_index<0>, 1};
        idym::variant<comparable, int> rhs{idym::in_place_index<0>, 0};
        idym_test::validate(!(lhs < rhs), "variant.relops.6");
        
        idym::get<0>(rhs).value = 2;
        idym_test::validate(lhs < rhs, "variant.relops.6");
    }
}
void run_7_8() {
    {
        const auto lhs = make_valueless<comparable>();
        const valueless_var_t<comparable> rhs{idym::in_place_index<2>, 1};
        idym_test::validate(!(lhs > rhs), "variant.relops.8");
    }
    {
        const valueless_var_t<comparable> lhs{idym::in_place_index<2>, 1};
        const auto rhs = make_valueless<comparable>();
        idym_test::validate(lhs > rhs, "variant.relops.8");
    }
    {
        const idym::variant<comparable, int> lhs{idym::in_place_index<0>, 1};
        const idym::variant<comparable, int> rhs{idym::in_place_index<1>, 1};
        idym_test::validate(!(lhs > rhs), "variant.relops.8");
    }
    {
        const idym::variant<comparable, int> lhs{idym::in_place_index<1>, 1};
        const idym::variant<comparable, int> rhs{idym::in_place_index<0>, 1};
        idym_test::validate(lhs > rhs, "variant.relops.8");
    }
    {
        idym::variant<comparable, int> lhs{idym::in_place_index<0>, 1};
        idym::variant<comparable, int> rhs{idym::in_place_index<0>, 0};
        idym_test::validate(lhs > rhs, "variant.relops.8");
        
        idym::get<0>(rhs).value = 2;
        idym_test::validate(!(lhs > rhs), "variant.relops.8");
    }
}
void run_9_10() {
    {
        const auto lhs = make_valueless<comparable>();
        const valueless_var_t<comparable> rhs{idym::in_place_index<2>, 1};
        idym_test::validate(lhs <= rhs, "variant.relops.10");
    }
    {
        const valueless_var_t<comparable> lhs{idym::in_place_index<2>, 1};
        const auto rhs = make_valueless<comparable>();
        idym_test::validate(!(lhs <= rhs), "variant.relops.10");
    }
    {
        const idym::variant<comparable, int> lhs{idym::in_place_index<0>, 1};
        const idym::variant<comparable, int> rhs{idym::in_place_index<1>, 1};
        idym_test::validate(lhs <= rhs, "variant.relops.10");
    }
    {
        const idym::variant<comparable, int> lhs{idym::in_place_index<1>, 1};
        const idym::variant<comparable, int> rhs{idym::in_place_index<0>, 1};
        idym_test::validate(!(lhs <= rhs), "variant.relops.10");
    }
    {
        idym::variant<comparable, int> lhs{idym::in_place_index<0>, 1};
        idym::variant<comparable, int> rhs{idym::in_place_index<0>, 0};
        idym_test::validate(!(lhs <= rhs), "variant.relops.10");
        
        idym::get<0>(rhs).value = 2;
        idym_test::validate(lhs <= rhs, "variant.relops.10");
        
        idym::get<0>(rhs).value = 1;
        idym_test::validate(lhs <= rhs, "variant.relops.10");
    }
}
void run_11_12() {
    {
        const auto lhs = make_valueless<comparable>();
        const valueless_var_t<comparable> rhs{idym::in_place_index<2>, 1};
        idym_test::validate(!(lhs >= rhs), "variant.relops.12");
    }
    {
        const valueless_var_t<comparable> lhs{idym::in_place_index<2>, 1};
        const auto rhs = make_valueless<comparable>();
        idym_test::validate(lhs >= rhs, "variant.relops.12");
    }
    {
        const idym::variant<comparable, int> lhs{idym::in_place_index<0>, 1};
        const idym::variant<comparable, int> rhs{idym::in_place_index<1>, 1};
        idym_test::validate(!(lhs >= rhs), "variant.relops.12");
    }
    {
        const idym::variant<comparable, int> lhs{idym::in_place_index<1>, 1};
        const idym::variant<comparable, int> rhs{idym::in_place_index<0>, 1};
        idym_test::validate(lhs >= rhs, "variant.relops.12");
    }
    {
        idym::variant<comparable, int> lhs{idym::in_place_index<0>, 1};
        idym::variant<comparable, int> rhs{idym::in_place_index<0>, 0};
        idym_test::validate(lhs >= rhs, "variant.relops.12");
        
        idym::get<0>(rhs).value = 2;
        idym_test::validate(!(lhs >= rhs), "variant.relops.12");
        
        idym::get<0>(rhs).value = 1;
        idym_test::validate(lhs >= rhs, "variant.relops.12");
    }
}

}

// [variant.visit]
namespace variant_visit {

template<std::size_t>
struct visit_type {};

struct visitor {
    template<typename... Ts>
    void* operator()(Ts&&...) const {
        return nullptr;
    }
    void* operator()(visit_type<1>& v, visit_type<3>&, const visit_type<5>&) const {
        return &v;
    }
};

void run_1_8() {
    {
        idym::variant<visit_type<0>, visit_type<1>> v1{idym::in_place_type<visit_type<1>>};
        idym::variant<visit_type<1>, visit_type<2>, visit_type<3>> v2{idym::in_place_type<visit_type<3>>};
        const idym::variant<visit_type<2>, visit_type<3>, visit_type<4>, visit_type<5>> v3{idym::in_place_type<visit_type<5>>};
        
        void* ret = idym::visit(visitor{}, v1, v2, v3);
        idym_test::validate(ret == &idym::get<1>(v1), "variant.visit.6");
        
        v2.emplace<0>();
        ret = idym::visit(visitor{}, v1, v2, v3);
        idym_test::validate(!ret, "variant.visit.6");
    }
    {
        const auto v1 = make_valueless();
        idym::variant<visit_type<6>, visit_type<7>> v2;
        idym::variant<visit_type<8>, visit_type<9>, visit_type<10>> v3;
        
        IDYM_VALIDATE_BAD_ACCESS("variant.visit.7", visit(visitor{}, v1, v2, v3));
    }
}

}

int main(int, char**) {
    variant_ctor::run_1_6();
    variant_ctor::run_7_9();
    variant_ctor::run_10_13();
    variant_ctor::run_14_19();
    variant_ctor::run_20_29();
    
    variant_dtor::run_1_2();
    
    variant_assign::run_1_5();
    variant_assign::run_6_10();
    variant_assign::run_11_16();
    
    variant_mod::run_1_18();
    
    variant_status::run_1_3();
    
    variant_swap::run_1_5();
    
    variant_get::run_1_2();
    variant_get::run_3_9();
    variant_get::run_10_13();
    
    variant_relops::run_1_2();
    variant_relops::run_3_4();
    variant_relops::run_5_6();
    variant_relops::run_7_8();
    variant_relops::run_9_10();
    variant_relops::run_11_12();
    
    variant_visit::run_1_8();
    return 0;
}
