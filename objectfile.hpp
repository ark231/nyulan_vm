#ifndef NYULAN_OBJECTFILE
#define NYULAN_OBJECTFILE
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <memory>
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

struct OptionalSection;

constexpr std::uint64_t CURRENT_OBJECTRILE_VERSION = 3;
struct ObjectFile {
    char magic[3 + 1];        //{'N','Y','U','\0'}(NULは読み取り時に追記される)
    std::uint8_t bom[2 + 1];  // 0x1100
    Endian endian;
    std::uint64_t version;
    std::uint16_t literal_data_size;
    std::vector<std::uint8_t> literal_datas;
    std::uint16_t global_label_num;
    struct Label {
        std::string real_name;  // NUL terminated in actual file
        std::uint64_t label_address;
    };
    std::vector<Label> global_labels;
    std::uint64_t instruction_set_version;
    std::uint64_t code_length;
    std::vector<OneStep> code;
    std::uint64_t num_optional_sections;
    std::vector<std::unique_ptr<OptionalSection>> optional_sections;

    ObjectFile(std::string objectfilename);
    ObjectFile() = default;

    Label find_label(std::string name);
    std::string pretty();

   private:
    template <typename T>
    T file_to_value(std::ifstream&);  //ファイルを読んだことによる変更を外に波及させたいので、参照
    template <typename T>
    T bytes_to_value(std::vector<std::uint8_t> bytes);  //中で変更するかもなので、参照ではなくコピーを受ける！必ず！
};
struct OptionalSection {
    std::string name;
    std::uint16_t datasize;
    std::vector<std::uint8_t> data;
};

struct DebugSection : public OptionalSection {
    using OptionalSection::OptionalSection;
    Address addr2line(Address step_address);
};
}  // namespace nyulan
#endif
