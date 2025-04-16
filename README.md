## Compliant C++14 implementation of `std::variant`

The implementation includes the variant itself and supplimentary standard library features not present in C++14.
The variant is defined as per the C++23 spec, with certain features omitted for the lack of language features dependending
on the language version in use(see below).

### Building
All the relevant headers are located in `include/idym`, compilation not necessary. The headers are relocatable as they are,
they only assume that the are all located in the same directory, for include purposes.

The CMake project provides an interface library `idym` with `include` added to the include directories. The project defaults to *C++14*, if
the language version is not set explicitly.  
Additionally, test targets can be disabled by switching `IDYM_BUILD_TESTS` to *OFF*.

### Usage
All definitions are located in `idym` namespace, it can be changed with a `IDYM_NAMESPACE=<namespace-name>` macro.
```cpp
#define IDYM_NAMESPACE foo_ns
#include <idym/variant.hpp>

void foo() {
    foo_ns::variant<int, char> variant;
    foo_ns::get<int>(variant);
}
```
Specializations of `std::swap` and `std::hash` can be disabled with by defining `IDYM_NOSTD_INTEROP`.

### Installation
The installation of a total of three(3) headers is left as an exercise to the reader.

### Language versions and features
Certain features of the implementation, as the spec defines it, will be unavailable in *C++14*. Below is a table listing the minimum
language version required for all affected features.
| Feature                                                                   | Required language version                |
| :------------------------------------------------------------------------ | :--------------------------------------- |
| Inline `in_place` and `variant_npos`                                      | *C++17(__cpp_inline_variables)*          |
| Constexpr `variant` destructor                                            | *C++20(__cpp_constexpr)*                 |
| *operator<=>* for `variant` and `monostate`                               | *C++20(__cpp_impl_three_way_comparison)* |
| Deprecated *volatile* flavors of `variant_alternative` and `variant_size` | *C++20*                                  |