#include <idym/variant.hpp>

void variant_ctor() {
    idym::variant<int> v;
    idym::variant<int> v2{v};
    static_assert(std::is_trivially_move_constructible<idym::variant<int>>::value, "ij");
}

int main(int argc, char** argv) {
    return 0;
}
