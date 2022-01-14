#include <algorithm>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <vector>

#include "nyulan.hpp"
namespace nyulan {
enum class Endian {
    LITTLE,
    BIG,
};
struct ObjectFile {
    char magic[3 + 1];        //{'N','Y','U','\0'}
    std::uint8_t bom[2 + 1];  // 0x1100
    Endian endian;
    std::uint64_t version;
    std::uint16_t literal_data_size;
    std::vector<std::uint8_t> literal_datas;
    std::uint16_t global_label_num;
    struct Label {
        std::string rael_name;  // NUL terminated in actual file
        std::uint64_t label_address;
    };
    std::vector<Label> global_labels;
    std::uint64_t code_length;
    std::vector<OneStep> code;

    ObjectFile(std::string);
    ObjectFile() = default;

   private:
    template <typename T>
    T file_to_value(std::ifstream&);  //ファイルを読んだことによる変更を外に波及させたいので、参照
    template <typename T>
    T bytes_to_value(std::vector<std::uint8_t> bytes);  //中で変更するかもなので、参照ではなくコピーを受ける！必ず！
};
}  // namespace nyulan
