#include "objectfile.hpp"

#include <bitset>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#include "logging.hpp"
namespace nyulan {
std::vector<std::uint8_t> read_file(std::ifstream&, size_t);
ObjectFile::ObjectFile(std::string objectfilename) {
    std::ifstream objectfile(objectfilename, std::ios_base::in | std::ios_base::binary);
    if (!objectfile.is_open()) {
        throw new std::runtime_error("error: couldn't open objectfile " + objectfilename);
    }
    objectfile >> std::setw(4) >> this->magic;
    TRIVIAL_LOG_WITH_FUNCNAME(debug) << this->magic;
    if (std::string(this->magic) != "NYU") {
        throw new std::runtime_error("given file is not nyulan object file");
    }
    objectfile >> std::setw(3) >> this->bom;
    switch (this->bom[0]) {
        case 0x00:
            this->endian = Endian::LITTLE;
            break;
        case 0x11:
            this->endian = Endian::BIG;
            break;
    }
    this->version = this->file_to_value<decltype(this->version)>(objectfile);
    TRIVIAL_LOG_WITH_FUNCNAME(debug) << this->version;
    this->literal_data_size = this->file_to_value<decltype(this->literal_data_size)>(objectfile);

    TRIVIAL_LOG_WITH_FUNCNAME(debug) << this->literal_data_size;
    this->literal_datas = read_file(objectfile, this->literal_data_size);
    TRIVIAL_LOG_WITH_FUNCNAME(debug) << "data_segment:\"" << this->literal_datas.data() << "\"";
    TRIVIAL_LOG_WITH_FUNCNAME(debug) << this->literal_datas.size();

    this->global_label_num = this->file_to_value<decltype(this->global_label_num)>(objectfile);
    TRIVIAL_LOG_WITH_FUNCNAME(debug) << "glabel_n:" << this->global_label_num;

    this->global_labels.reserve(this->global_label_num);
    for (auto i = 0; i < this->global_label_num; i++) {
        Label label;
        std::getline(objectfile, label.rael_name, '\0');
        label.label_address = this->file_to_value<decltype(label.label_address)>(objectfile);
        this->global_labels.push_back(label);
    }

    this->code_length = this->file_to_value<decltype(this->code_length)>(objectfile);
    this->code.reserve(this->code_length);
    for (size_t i = 0; i < this->code_length; i++) {
        auto onestep = this->file_to_value<decltype(this->code)::value_type::ValueType>(objectfile);  //エンディアン
        this->code.push_back(onestep);
    }
    objectfile.close();
}
template <typename T>
T ObjectFile::bytes_to_value(std::vector<std::uint8_t> bytes) {
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
template <typename T>
T ObjectFile::file_to_value(std::ifstream& objectfile) {
    return this->bytes_to_value<T>(read_file(objectfile, sizeof(std::declval<T>())));
}
std::vector<std::uint8_t> read_file(std::ifstream& file, size_t n) {
    std::vector<std::uint8_t> result;
    result.reserve(n);
    for (size_t i = 0; i < n; i++) {
        result.push_back(file.get());
    }
    return result;
}
}  // namespace nyulan
