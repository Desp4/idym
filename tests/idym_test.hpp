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
}

#endif
