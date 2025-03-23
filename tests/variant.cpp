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

// [variant.ctor]
namespace variant_ctor {

struct ndef_ctor {
    ndef_ctor() = delete;
};
struct ncopy_ctor {
    ncopy_ctor(const ncopy_ctor&) = delete;
};
struct copy_ctor_throws {
    copy_ctor_throws() = default;
    copy_ctor_throws(const copy_ctor_throws&) { throw test_exception{}; }
};

struct def_ctor_throws {
    def_ctor_throws() { throw test_exception{}; }
};
struct def_ctor {
    static constexpr int expected = 1337;
    def_ctor() : value{expected} {}
    
    int value;
};
struct copy_ctor {
    copy_ctor() = default;
    copy_ctor(const copy_ctor& other) : value{other.value} {}
    
    int value;
};

struct valueless_move_throws {
    valueless_move_throws() = default;
    valueless_move_throws(const valueless_move_throws&) = default;
    valueless_move_throws(valueless_move_throws&&) { throw test_exception{}; }
    valueless_move_throws& operator=(valueless_move_throws&&) { throw test_exception{}; return *this; }
};

void run_1_6() {
    static_assert(!std::is_default_constructible<variant<ndef_ctor, int>>::value, "variant.ctor.1");
    static_assert(std::is_default_constructible<variant<int, ndef_ctor>>::value, "variant.ctor.1");
    
    static_assert(!std::is_nothrow_default_constructible<variant<def_ctor_throws>>::value, "variant.ctor.6");
    static_assert(std::is_nothrow_default_constructible<variant<int>>::value, "variant.ctor.6");
    
    {
        variant<def_ctor> v;
        validate(!v.valueless_by_exception(), "variant.ctor.4");
        validate(v.index() == 0, "variant.ctor.4");
        validate(idym::get<0>(v).value == def_ctor::expected, "variant.ctor.3");
    }

    
    IDYM_VALIDATE_EXCEPTION("variant.ctor.5", variant<def_ctor_throws, int>{});
}
void run_7_9() {
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
        validate(idym::get<0>(v2).value == 1337, "variant.ctor.7");
    }
    {
        variant<valueless_move_throws, int> v1;
        variant<valueless_move_throws, int> v2{idym::in_place_index<1>, 0};
        try { v2 = std::move(v1); } catch (test_exception) {}
        
        variant<valueless_move_throws, int> v3{v2};
        validate(v3.valueless_by_exception(), "variant.ctor.7");
    }
    {
        variant<copy_ctor_throws, int> v1;
        IDYM_VALIDATE_EXCEPTION("variant.ctor.8", variant<copy_ctor_throws, int>{v1});
    }
}

}


int main(int argc, char** argv) {
    variant_ctor::run_1_6();
    variant_ctor::run_7_9();
    return 0;
}
