#include <iostream>
#include "DynamicFixed.hpp"

int main() {
    // Explicit usage via the numeric namespace
    numeric::DynamicFixed a(8.75, 13, 5);
    numeric::DynamicFixed b(8.75, 13, 5);

    numeric::DynamicFixed sum = a + b;
    numeric::DynamicFixed target = sum.cast_to(16, 6);

    std::cout << "Target Raw: " << target.raw_value << "\n"; // Outputs 1120
    return 0;
}
