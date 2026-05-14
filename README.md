# Base256

**IEEE 754 binary256 (256-bit / octuple-precision) floating-point library for C++23**

Header-only implementation of the proposed IEEE 754 binary256 format:
- 1 sign bit
- 19 exponent bits (bias 262143)
- 236 fraction bits (237-bit significand precision)

## Features
- Exact bit-level representation and special value handling (±0, ±∞, NaN, subnormals)
- Construction from `double`, `long double`, and decimal strings
- User-defined literal `_f256`
- Full three-way comparison (`operator<=>`)
- Basic arithmetic (`+`, `-`, `*`, `/`) with correct special-value propagation
- `std::numeric_limits<float256>` specialization
- `to_double()` and `to_string()` conversions

> **Note**: Arithmetic currently uses `long double` approximation for practicality (full correctly-rounded 256-bit soft-float arithmetic is planned for a future release).

## Usage

```cpp
#include <base256/float256.hpp>
#include <iostream>

int main() {
    using namespace base256::literals;

    auto pi = 3.14159265358979323846264338327950288_f256;
    auto two = 2.0_f256;
    auto area = pi * two * two;

    std::cout << "Area ≈ " << area.to_string(20) << std::endl;
    return 0;
}
```

## Building

```bash
git clone https://github.com/MyGrokOrg/Base256.git
cd Base256
cmake -B build -S .
cmake --build build
```

## Namespace & Include
- Namespace: `base256`
- Include: `#include <base256/float256.hpp>`

## License
MIT — see LICENSE file.

Created by Craig D. Mansfield, PhD, EI (MyGrokOrg). Contributions welcome!