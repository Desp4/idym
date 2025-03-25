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
    valueless_helper& operator=(valueless_helper&&) { throw test_exception{}; return *this; }
};
using valueless_var_t = variant<valueless_helper, int>;

valueless_var_t make_valueless() {
    valueless_var_t v1;
    valueless_var_t v2{idym::in_place_index<1>, 0};
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
        auto v1 = make_valueless();
        valueless_var_t v2{v1};
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
        auto v1 = make_valueless();
        valueless_var_t v2{std::move(v1)};
        validate(v2.valueless_by_exception(), "variant.ctor.11");
    }
    {
        variant<move_ctor_throws, int> v1;
        IDYM_VALIDATE_EXCEPTION("variant.ctor.12", variant<move_ctor_throws, int>{std::move(v1)});
    }
}
void run_14_19() {
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
    
    static_assert(std::is_constructible<variant<int_ctor, int, char_ctor>, char>::value, "variant.ctor.15");
    static_assert(!std::is_constructible<variant<int_ctor, char_ctor>, char>::value, "variant.ctor.15");
    static_assert(!std::is_constructible<variant<int_ctor, int_ctor>, int>::value, "variant.ctor.15");
    static_assert(std::is_constructible<variant<int_ctor, int_ctor, ptr_ctor>, void*>::value, "variant.ctor.15");
    
    static_assert(std::is_nothrow_constructible<variant<int, void*>, int>::value, "variant.ctor.19");
    static_assert(std::is_nothrow_constructible<variant<int_ctor, ptr_ctor>, int>::value, "variant.ctor.19");
    static_assert(!std::is_nothrow_constructible<variant<int_ctor, ptr_ctor>, void*>::value, "variant.ctor.19");
    
    // 1. forwarding(move/copy) 2. alt selection 3. exceptions
}

}


int main(int argc, char** argv) {
    variant_ctor::run_1_6();
    variant_ctor::run_7_9();
    variant_ctor::run_10_13();
    variant_ctor::run_14_19();
    return 0;
}
