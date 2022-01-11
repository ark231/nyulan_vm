#include "objectfile.hpp"

#include <iomanip>
#include <stdexcept>
#include <string>
#include <utility>
namespace nyulan {
ObjectFile::ObjectFile(std::string objectfilename) {
    std::ifstream objectfile(objectfilename, std::ios_base::in | std::ios_base::binary);
    if (!objectfile.is_open()) {
        throw new std::runtime_error("error: couldn't open objectfile " + objectfilename);
    }
    objectfile >> std::setw(3) >> this->magic;
    if (std::string(this->magic) != "NYU") {
        throw new std::runtime_error("given file is not nyulan object file");
    }
    objectfile >> std::setw(2) >> this->bom;
    switch (this->bom[0]) {
        case 0x00:
            this->endian = Endian::LITTLE;
            break;
        case 0x11:
            this->endian = Endian::BIG;
            break;
    }
    this->version = this->file_to_value<decltype(this->version)>(objectfile);
    this->literal_data_size = this->file_to_value<decltype(this->literal_data_size)>(objectfile);

    std::uint8_t* literal_datas = new std::uint8_t[this->literal_data_size];
    objectfile >> std::setw(this->literal_data_size) >> literal_datas;
    this->literal_datas = std::vector<std::uint8_t>(literal_datas, literal_datas + (this->literal_data_size - 1));
    delete[] literal_datas;

    this->global_label_num = this->file_to_value<decltype(this->global_label_num)>(objectfile);

    this->global_labels.reserve(this->global_label_num);
    for (auto i = 0; i < this->global_label_num; i++) {
        Label label;
        std::getline(objectfile, label.lael_name, '\0');
        label.label_address = this->file_to_value<decltype(label.label_address)>(objectfile);
        this->global_labels.push_back(label);
    }

    this->code_length = this->file_to_value<decltype(this->code_length)>(objectfile);
    this->code.reserve(this->code_length);
    for (size_t i = 0; i < this->code_length; i++) {
        auto onestep = this->file_to_value<decltype(this->code)::value_type>(objectfile);  //エンディアン
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
    std::size_t value_len = sizeof(std::declval<T>());
    std::uint8_t value[value_len];
    objectfile >> std::setw(value_len) >> value;
    return this->bytes_to_value<T>(std::vector<std::uint8_t>(value, value + (value_len - 1)));
}
}  // namespace nyulan
