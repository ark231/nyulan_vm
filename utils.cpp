
#include "utils.hpp"

#include <cstdint>
namespace nyulan {
namespace utils {
Endian check_native_endian() {
    std::uint16_t bom = 0x1100;
    Endian result;
    switch (reinterpret_cast<std::uint8_t*>(&bom)[0]) {
        case 0x00:
            result = Endian::LITTLE;
            break;
        case 0x11:
            result = Endian::BIG;
            break;
    }
    return result;
}
}  // namespace utils
}  // namespace nyulan
