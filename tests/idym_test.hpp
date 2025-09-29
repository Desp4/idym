#ifndef IDYM_TEST_H
#define IDYM_TEST_H

#include <cassert>
#include <iostream>

#define IDYM_VALIDATE_EXCEPTION_GENERIC(STR, EXCEPTION_TYPE, ...) \
    do { \
        bool caught = false; \
        try { __VA_ARGS__; } \
        catch (EXCEPTION_TYPE) { caught = true; } \
        ::idym_test::validate(caught, STR); \
    } while(false)
#define IDYM_VALIDATE_EXCEPTION(STR, ...) IDYM_VALIDATE_EXCEPTION_GENERIC(STR, ::idym_test::test_exception, __VA_ARGS__)

namespace idym_test {
struct test_exception {};

inline void validate(bool condition, const char* str) {
    if (condition)
        return;
    
    std::cout << str << '\n';
    assert(condition);
    std::exit(4);
}

struct ndef_ctor {
    ndef_ctor() = delete;
};
struct def_ctor {
    int value = 1337;
};
struct def_ctor_throws {
    def_ctor_throws() { throw test_exception{}; }
};

struct copy_ctor {
    copy_ctor() = default;
    copy_ctor(const copy_ctor& other) : value{other.value + 1} {}
    
    int value = 0;
};
struct copy_ctor_throws {
    copy_ctor_throws() = default;
    copy_ctor_throws(const copy_ctor_throws&) { throw idym_test::test_exception{}; }
};
struct ncopy_ctor {
    ncopy_ctor(const ncopy_ctor&) = delete;
};

struct move_ctor {
    move_ctor() = default;
    move_ctor(move_ctor&& other) noexcept : value{other.value + 1} {}
    
    int value{};
};
struct move_ctor_throws {
    move_ctor_throws() = default;
    move_ctor_throws(move_ctor_throws&&) { throw idym_test::test_exception{}; }
};
struct nmove_ctor {
    nmove_ctor(nmove_ctor&&) = delete;
};
}

#endif
