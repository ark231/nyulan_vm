#ifndef NYULAN_OBJECTFILE_TEMPLATES
#define NYULAN_OBJECTFILE_TEMPLATES
#include "objectfile.hpp"  //こっちからインクルードされるからいらないが、リンターのエラーを抑えるため
namespace nyulan {
template <typename T>
T ObjectFile::bytes_to_value(std::vector<std::uint8_t> bytes) const {
    if (bytes.size() < sizeof(T)) {
        throw new std::invalid_argument("provided " + std::to_string(bytes.size()) +
                                        " bytes of bytearray is not long enough to construct" + typeid(T).name());
    }
    std::uint16_t bom = 0x1100;
    Endian native_endian;
    switch (reinterpret_cast<char*>(&bom)[0]) {
        case 0x00:
            native_endian = Endian::LITTLE;
            break;
        case 0x11:
            native_endian = Endian::BIG;
            break;
    }
    if (native_endian != this->endian) {
        std::reverse(bytes.begin(), bytes.end());  //入力をひっくり返す
    }
    return *reinterpret_cast<T*>(bytes.data());
}
}  // namespace nyulan
#endif
