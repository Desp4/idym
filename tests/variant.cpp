#include <iostream>
#include <cassert>

#include <idym/variant.hpp>

using idym::variant;

// common
#define IDYM_VALIDATE_EXCEPTION(STR, ...) \
    do { \
        bool caught = false; \
        try { __VA_ARGS__; } \
        catch (test_exception) { caught = true; } \
        validate(caught, STR); \
    } while(false)

struct test_exception {};

void validate(bool condition, const char* str) {
    if (condition)
        return;
    std::cout << str << '\n';
    
    assert(condition);
    std::exit(4);
}

struct valueless_helper {
    valueless_helper() = default;
    valueless_helper(const valueless_helper&) = default;
    valueless_helper(valueless_helper&&) { throw test_exception{}; }
    valueless_helper& operator=(const valueless_helper&) = default;
    valueless_helper& operator=(valueless_helper&&) { throw test_exception{}; return *this; }
};
template<typename... Ts>
using valueless_var_t = variant<valueless_helper, int, Ts...>;

template<typename... Ts>
valueless_var_t<Ts...> make_valueless() {
    valueless_var_t<Ts...> v1;
    valueless_var_t<Ts...> v2{idym::in_place_index<1>, 0};
    try { v2 = std::move(v1); } catch (test_exception) {}
    return v2;
}

// [variant.ctor]
namespace variant_ctor {

void run_1_6() {
    struct def_ctor {
        def_ctor() : value{1337} {}
        
        int value;
    };
    struct def_ctor_throws {
        def_ctor_throws() { throw test_exception{}; }
    };
    struct ndef_ctor {
        ndef_ctor() = delete;
    };

    static_assert(!std::is_default_constructible<variant<ndef_ctor, int>>::value, "variant.ctor.1");
    static_assert(std::is_default_constructible<variant<int, ndef_ctor>>::value, "variant.ctor.1");
    
    static_assert(!std::is_nothrow_default_constructible<variant<def_ctor_throws>>::value, "variant.ctor.6");
    static_assert(std::is_nothrow_default_constructible<variant<int>>::value, "variant.ctor.6");
    
    {
        variant<def_ctor> v;
        validate(!v.valueless_by_exception(), "variant.ctor.4");
        validate(v.index() == 0, "variant.ctor.4");
        validate(idym::get<0>(v).value == 1337, "variant.ctor.3");
    }

    IDYM_VALIDATE_EXCEPTION("variant.ctor.5", variant<def_ctor_throws, int>{});
}
void run_7_9() {
    struct copy_ctor {
        copy_ctor() = default;
        copy_ctor(const copy_ctor& other) : value{other.value + 1} {}
        
        int value;
    };
    struct copy_ctor_throws {
        copy_ctor_throws() = default;
        copy_ctor_throws(const copy_ctor_throws&) { throw test_exception{}; }
    };
    struct ncopy_ctor {
        ncopy_ctor(const ncopy_ctor&) = delete;
    };

    static_assert(std::is_copy_constructible<variant<int>>::value, "variant.ctor.9");
    static_assert(std::is_trivially_copy_constructible<variant<int>>::value, "variant.ctor.9");

    static_assert(std::is_copy_constructible<variant<int, copy_ctor>>::value, "variant.ctor.9");
    static_assert(!std::is_trivially_copy_constructible<variant<int, copy_ctor>>::value, "variant.ctor.9");

    static_assert(!std::is_copy_constructible<variant<int, ncopy_ctor>>::value, "variant.ctor.9");
    
    {
        variant<copy_ctor, int> v1;
        idym::get<0>(v1).value = 1337;
        
        variant<copy_ctor, int> v2{v1};
        validate(v1.index() == v2.index(), "variant.ctor.7");
        validate(idym::get<0>(v2).value == 1338, "variant.ctor.7");
    }
    {
        auto v1 = make_valueless<int>();
        valueless_var_t<int> v2{v1};
        validate(v2.valueless_by_exception(), "variant.ctor.7");
    }
    {
        variant<copy_ctor_throws, int> v1;
        IDYM_VALIDATE_EXCEPTION("variant.ctor.8", variant<copy_ctor_throws, int>{v1});
    }
}
void run_10_13() {
    struct move_ctor {
        move_ctor() = default;
        move_ctor(move_ctor&& other) noexcept : value{other.value + 1} {}
        
        int value;
    };
    struct move_ctor_throws {
        move_ctor_throws() = default;
        move_ctor_throws(move_ctor_throws&&) { throw test_exception{}; }
    };
    struct nmove_ctor {
        nmove_ctor(nmove_ctor&&) = delete;
    };

    static_assert(std::is_move_constructible<variant<int>>::value, "variant.ctor.10");
    static_assert(std::is_trivially_move_constructible<variant<int>>::value, "variant.ctor.13");
    static_assert(std::is_nothrow_move_constructible<variant<int>>::value, "variant.ctor.13");
    
    static_assert(std::is_move_constructible<variant<int, move_ctor>>::value, "variant.ctor.10");
    static_assert(!std::is_trivially_move_constructible<variant<int, move_ctor>>::value, "variant.ctor.13");
    static_assert(std::is_nothrow_move_constructible<variant<int, move_ctor>>::value, "variant.ctor.13");
    
    static_assert(std::is_move_constructible<variant<int, move_ctor_throws>>::value, "variant.ctor.10");
    static_assert(!std::is_trivially_move_constructible<variant<int, move_ctor_throws>>::value, "variant.ctor.13");
    static_assert(!std::is_nothrow_move_constructible<variant<int, move_ctor_throws>>::value, "variant.ctor.13");
    
    static_assert(!std::is_move_constructible<variant<int, nmove_ctor>>::value, "variant.ctor.10");
    
    {
        variant<move_ctor, int> v1;
        idym::get<0>(v1).value = 1337;
        
        variant<move_ctor, int> v2{std::move(v1)};
        validate(v1.index() == v2.index(), "variant.ctor.11");
        validate(idym::get<0>(v2).value == 1338, "variant.ctor.11");
    }
    {
        auto v1 = make_valueless<int>();
        valueless_var_t<int> v2{std::move(v1)};
        validate(v2.valueless_by_exception(), "variant.ctor.11");
    }
    {
        variant<move_ctor_throws, int> v1;
        IDYM_VALIDATE_EXCEPTION("variant.ctor.12", variant<move_ctor_throws, int>{std::move(v1)});
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
        ptr_ctor(void*) { throw test_exception{}; }
    };
    
    struct move_copy_ctor {
        move_copy_ctor(int init_value = 0) : value{init_value} {}
        move_copy_ctor(const move_copy_ctor& other) : state_flag{1}, value{other.value} {}
        move_copy_ctor(move_copy_ctor&& other) : state_flag{2}, value{other.value} {}
        
        int state_flag = 0;
        int value = 0;
    };
    
    static_assert(std::is_constructible<variant<int_ctor, int, char_ctor>, char>::value, "variant.ctor.15");
    static_assert(!std::is_constructible<variant<int_ctor, char_ctor>, char>::value, "variant.ctor.15");
    static_assert(!std::is_constructible<variant<int_ctor, int_ctor>, int>::value, "variant.ctor.15");
    static_assert(std::is_constructible<variant<int_ctor, int_ctor, ptr_ctor>, void*>::value, "variant.ctor.15");
    
    static_assert(std::is_nothrow_constructible<variant<int, void*>, int>::value, "variant.ctor.19");
    static_assert(std::is_nothrow_constructible<variant<int_ctor, ptr_ctor>, int>::value, "variant.ctor.19");
    static_assert(!std::is_nothrow_constructible<variant<int_ctor, ptr_ctor>, void*>::value, "variant.ctor.19");

    {
        variant<void*, move_copy_ctor> v{42};
        validate(idym::holds_alternative<move_copy_ctor>(v), "variant.ctor.17");
        validate(idym::get<1>(v).value == 42, "variant.ctor.16");
    }
    {
        move_copy_ctor value{42};
        variant<void*, move_copy_ctor> v{std::move(value)};
        validate(idym::holds_alternative<move_copy_ctor>(v), "variant.ctor.17");
        validate(idym::get<1>(v).value == 42, "variant.ctor.17");
        validate(idym::get<1>(v).state_flag == 2, "variant.ctor.17");
    }
    IDYM_VALIDATE_EXCEPTION("variant.ctor.18", variant<ptr_ctor, int>{nullptr});
}
void run_20_29() {
    struct init_list_type {
        init_list_type(std::initializer_list<int> ints, int v) : ints_size{ints.size()}, value{v} {}
        
        std::size_t ints_size;
        int value;
    };
    struct init_list_type_throws {
        init_list_type_throws(std::initializer_list<int>) { throw test_exception{}; }
    };
    struct throws {
        throws(int, int) { throw test_exception{}; }
    };

    static_assert(!std::is_constructible<variant<int, int>, idym::in_place_type_t<int>, int>::value, "variant.ctor.20.1");
    static_assert(std::is_constructible<variant<int, void*>, idym::in_place_type_t<int>, int>::value, "variant.ctor.20.1");
    
    static_assert(!std::is_constructible<variant<int, void*>, idym::in_place_type_t<int>, void*>::value, "variant.ctor.20.2");
    static_assert(std::is_constructible<variant<int, void*>, idym::in_place_type_t<int>, int>::value, "variant.ctor.20.2");
    
    static_assert(!std::is_constructible<variant<init_list_type, init_list_type>, idym::in_place_type_t<init_list_type>, std::initializer_list<int>, int>::value, "variant.ctor.25.1");
    static_assert(std::is_constructible<variant<init_list_type, int>, idym::in_place_type_t<init_list_type>, std::initializer_list<int>, int>::value, "variant.ctor.25.1");
    
    static_assert(!std::is_constructible<variant<init_list_type, int>, idym::in_place_type_t<init_list_type>, std::initializer_list<int>, void*>::value, "variant.ctor.25.2");
    static_assert(std::is_constructible<variant<init_list_type, int>, idym::in_place_type_t<init_list_type>, std::initializer_list<int>, int>::value, "variant.ctor.25.2");
    
    {
        variant<int, void*> v{idym::in_place_type<int>, 4};
        validate(idym::holds_alternative<int>(v), "variant.ctor.22");
        validate(idym::get<0>(v) == 4, "variant.ctor.21");
    }
    {
        variant<int, init_list_type> v{idym::in_place_type<init_list_type>, {1, 2, 3}, 4};
        validate(idym::holds_alternative<init_list_type>(v), "variant.ctor.27");
        validate(idym::get<1>(v).ints_size == 3, "variant.ctor.26");
        validate(idym::get<1>(v).value == 4, "variant.ctor.26");
    }
    IDYM_VALIDATE_EXCEPTION("variant.ctor.23", variant<throws, int>{idym::in_place_type<throws>, 1, 2});
    IDYM_VALIDATE_EXCEPTION("variant.ctor.28", variant<init_list_type_throws, int>{idym::in_place_type<init_list_type_throws>, {1, 2}});
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
    
    static_assert(std::is_trivially_destructible<variant<int, int>>::value, "variant.dtor.2");
    static_assert(!std::is_trivially_destructible<variant<int, dtor_type>>::value, "variant.dtor.2");
    
    {
        bool dtor_flag = false;
        {
            variant<int, dtor_type> v{idym::in_place_type<dtor_type>, &dtor_flag};
        }
        validate(dtor_flag, "variant.dtor.1");
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
    {
        int dtor_count = 0;
        auto v1 = make_valueless<assign_type<0>>();
        auto v2 = make_valueless<assign_type<0>>();
        valueless_var_t<assign_type<0>> v3{idym::in_place_type<assign_type<0>>, &dtor_count};
        
        v2 = v1;
        v3 = v1;
        validate(v2.valueless_by_exception(), "variant.assign.2.1");
        validate(v3.valueless_by_exception(), "variant.assign.2.2");
        validate(dtor_count == 1, "variant.assign.2.2");
    }
    {
        int dtor_count = 0;
        variant<assign_type<0>, int> v1{&dtor_count};
        variant<assign_type<0>, int> v2{&dtor_count};
        
        auto& v2_ret = v2 = v1;
        validate(&v2_ret == &v2, "variant.assign.4");
        
        validate(v1.index() == v2.index(), "variant.assign.3");
        validate(idym::get<0>(v2).state == 3, "variant.assign.2.3");
        validate(dtor_count == 0, "variant.assign.2.3");
    }
    {
        int dtor_count1 = 0;
        int dtor_count2 = 0;
        int dtor_count3 = 0;
        variant<assign_type<0, true>, assign_type<0, false>> v1{idym::in_place_type<assign_type<0, true>>, &dtor_count1};
        variant<assign_type<0, true>, assign_type<0, false>> v2{idym::in_place_type<assign_type<0, true>>, &dtor_count2};
        variant<assign_type<0, true>, assign_type<0, false>> v3{idym::in_place_type<assign_type<0, false>>, &dtor_count3};
        
        v1 = v3;
        validate(v1.index() == v3.index(), "variant.assign.3");
        validate(idym::get<1>(v1).state == 1, "variant.assign.2.4");
        
        v1 = v2;
        validate(v1.index() == v2.index(), "variant.assign.3");
        validate(idym::get<0>(v1).state == 2, "variant.assign.2.5");
        
        validate(dtor_count1 == 1, "variant.assign.2.4");
        validate(dtor_count2 == 1, "variant.assign.2.5");
        validate(dtor_count3 == 1, "variant.assign.2.5");
    }
}

}


int main(int argc, char** argv) {
    variant_ctor::run_1_6();
    variant_ctor::run_7_9();
    variant_ctor::run_10_13();
    variant_ctor::run_14_19();
    variant_ctor::run_20_29();
    
    variant_dtor::run_1_2();
    
    variant_assign::run_1_5();
    return 0;
}
