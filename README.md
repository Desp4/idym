## Compliant C++14 implementation of `std::variant` and `std::expected`

The implementation includes the classes themselves and supplementary standard library entities not present in C++14.
Both `std::variant` and `std::expected` are defined as per the C++23 spec, with certain features omitted for the lack of language features dependending
on the language version in use(see below).

### Building
All the relevant headers are located in `include/idym`, compilation not necessary. The headers are relocatable as they are,
they only assume that they are all located in the same directory, for include purposes.

The CMake project provides an interface library `idym` with `include` added to the include directories. The project defaults to *C++14*, if
the language version is not set explicitly.  
Additionally, test targets can be disabled by switching `IDYM_BUILD_TESTS` to *OFF*.

### Usage
All definitions are located in `idym` namespace, it can be changed with a `IDYM_NAMESPACE=<namespace-name>` definition.
```cpp
#define IDYM_NAMESPACE foo_ns
#include <idym/variant.hpp>

void foo() {
    foo_ns::variant<int, char> variant;
    foo_ns::get<int>(variant);
}
```
Specializations of `std::swap` and `std::hash` can be disabled by defining `IDYM_NOSTD_INTEROP`.

### Installation
Copying the include directory to a desired location is left as an exercise to the reader.

### Language versions and features
Certain features of the implementation, as the spec defines it, will be unavailable in *C++14*. Below are the tables listing the minimum
language version required for all affected features.

#### `std::variant`
| Feature                                                                   | Required language version                |
| :------------------------------------------------------------------------ | :--------------------------------------- |
| Inline `in_place` and `variant_npos`                                      | *C++17(__cpp_inline_variables)*          |
| Constexpr non-trivial `variant` destructor                                | *C++20(__cpp_constexpr)*                 |
| *operator<=>* for `variant` and `monostate`                               | *C++20(__cpp_impl_three_way_comparison)* |
| Deprecated *volatile* flavors of `variant_alternative` and `variant_size` | *C++20*                                  |

#### `std::expected`
| Feature                                                    | Required language version       |
| :--------------------------------------------------------- | :------------------------------ |
| Inline `unexpect`                                          | *C++17(__cpp_inline_variables)* |
| Constexpr `bad_expected_access::what`                      | *C++20(__cpp_constexpr)*        |
| Fully constexpr `expected::operator=` and `expected::swap` | *C++20(__cpp_constexpr)*        |
| Constexpr non-trivial `expected` destructor                | *C++20(__cpp_constexpr)*        |
| `unexpected` deduction guide                               | *C++17(__cpp_deduction_guides)* |

#### Compiler support
Below is a table of compiler versions the implementation is confirmed to support. Not every release
in those ranges was tested, but just the oldest one I had access to and felt the need to support,
the most recent version and a handful of versions in between that I was in the mood to verify.

| Compiler  | Version range                  |
| :-------- | :----------------------------- |
| **MSVC**  | *[19.16.27050; 19.39.33521.0]* |
| **Clang** | *[9.0.0; 21.1.0]*              |
| **GCC**   | *[6.2; 15.2]*                  |