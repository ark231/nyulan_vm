#include "objectfile.hpp"

#include <bitset>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <cmath>
#include <iomanip>
#include <magic_enum.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#include "logging.hpp"
namespace nyulan {
std::vector<std::uint8_t> read_file(std::ifstream&, size_t);

template <typename T>
T ObjectFile::file_to_value(std::ifstream& objectfile) {
    return this->bytes_to_value<T>(read_file(objectfile, sizeof(std::declval<T>())));
}
template <>  // NULL終端文字列を取り出す特殊化
std::string ObjectFile::file_to_value(std::ifstream& objectfile) {
    std::string result;
    std::uint8_t char_read;
    do {
        char_read = objectfile.get();
        result.push_back(char_read);
    } while (char_read != '\0');
    result.pop_back();  // NOTE: 末尾に\0が含まれてしまっているので、削除
    return result;
}

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
    if (this->version > CURRENT_OBJECTRILE_VERSION) {
        throw std::runtime_error("the format is newer than this program");
    }

    TRIVIAL_LOG_WITH_FUNCNAME(debug) << this->literal_data_size;
    this->literal_datas = read_file(objectfile, this->literal_data_size);
    TRIVIAL_LOG_WITH_FUNCNAME(debug) << "data_segment:\"" << this->literal_datas.data() << "\"";
    TRIVIAL_LOG_WITH_FUNCNAME(debug) << this->literal_datas.size();

    this->global_label_num = this->file_to_value<decltype(this->global_label_num)>(objectfile);
    TRIVIAL_LOG_WITH_FUNCNAME(debug) << "glabel_n:" << this->global_label_num;

    this->global_labels.reserve(this->global_label_num);
    for (auto i = 0; i < this->global_label_num; i++) {
        Label label;
        std::getline(objectfile, label.real_name, '\0');
        label.label_address = this->file_to_value<decltype(label.label_address)>(objectfile);
        this->global_labels.push_back(label);
    }
    if (this->version >= 2) {
        this->instruction_set_version = this->file_to_value<decltype(this->instruction_set_version)>(objectfile);
    } else {
        this->instruction_set_version = 1;  // NOTE: obj-v1のときは命令セットはv1しかなかった
    }

    this->code_length = this->file_to_value<decltype(this->code_length)>(objectfile);
    this->code.reserve(this->code_length);
    for (size_t i = 0; i < this->code_length; i++) {
        auto onestep = this->file_to_value<decltype(this->code)::value_type::ValueType>(objectfile);  //エンディアン
        this->code.push_back(onestep);
    }
    if (this->version >= 3) {
        this->num_optional_sections = this->file_to_value<decltype(this->num_optional_sections)>(objectfile);
    } else {
        this->num_optional_sections = 0;  // NOTE: obj-v2まではoptional sectionはなかった
    }
    for (size_t i = 0; i < this->num_optional_sections; i++) {
        auto section = std::shared_ptr<OptionalSection>(new OptionalSection);
        section->parent = this;
        section->name = this->file_to_value<std::string>(objectfile);
        TRIVIAL_LOG_WITH_FUNCNAME(debug) << "new section with name \"" << section->name << "\"";
        section->datasize = this->file_to_value<decltype(OptionalSection::datasize)>(objectfile);
        TRIVIAL_LOG_WITH_FUNCNAME(debug) << "new section with datasize" << section->datasize;
        section->data = read_file(objectfile, section->datasize);
        this->optional_sections.push_back(section);
    }
    objectfile.close();
}  // namespace nyulan
ObjectFile::Label ObjectFile::find_label(std::string name) {
    for (const auto& label : this->global_labels) {
        if (label.real_name == name) {
            return label;
        }
    }
    throw std::runtime_error("label \"" + name + "\" not found");
}
std::shared_ptr<ObjectFile::OptionalSection> ObjectFile::find_section(std::string name) {
    TRIVIAL_LOG_WITH_FUNCNAME(debug) << "search for section with name\"" << name << "\"";
    for (const auto& section : this->optional_sections) {
        TRIVIAL_LOG_WITH_FUNCNAME(debug) << "current name:\"" << section->name << "\"";
        if (section->name == name) {
            return section;
        }
    }
    throw std::runtime_error("section \"" + name + "\" not found");
}
std::string ObjectFile::pretty() {
    std::stringstream result;
    result << "nyulan objectfile v" << this->version << std::endl;
    result << "static data size:" << this->literal_data_size << std::endl;
    result << "static data:\"" << this->literal_datas.data() << "\"" << std::endl;  // TODO: 改行などをエスケープ
    result << "global labels:{" << std::endl;
    for (const auto& label : this->global_labels) {
        result << "    \"" << label.real_name << "\" => @" << label.label_address << " ," << std::endl;
    }
    result << "}" << std::endl;
    result << "instruction set version:" << this->instruction_set_version << std::endl;
    result << "entry point:@" << this->find_label("_start").label_address << std::endl;
    auto max_code_addr_width = std::floor(std::log10(this->code_length)) + 1;  // NOTE: 数学的考察が必要
    for (size_t i = 0; i < this->code_length; i++) {
        result << std::dec << "@" << i << "  ";
        // HACK:ループで空白を追加しているが、多分一行でやる方法があるはず
        for (auto j = 0; j < (max_code_addr_width - (i == 0 ? 1 : std::floor(std::log10(i)) + 1)); j++) {
            result << " ";
        }
        auto step = this->code[i];
        auto opecode = ((step & 0b1111'1111'0000'0000) >> 8).value();
        result << magic_enum::enum_name(magic_enum::enum_cast<Instruction>(opecode).value()) << " ";
        switch (opecode) {
            case static_cast<std::uint8_t>(Instruction::RET):
                break;
            case static_cast<std::uint8_t>(Instruction::PUSHL):
                result << std::hex << static_cast<uint16_t>(step & 0b1111'1111);
                break;
            case static_cast<std::uint8_t>(Instruction::PUSHR8):
            case static_cast<std::uint8_t>(Instruction::PUSHR16):
            case static_cast<std::uint8_t>(Instruction::PUSHR32):
            case static_cast<std::uint8_t>(Instruction::PUSHR64):
            case static_cast<std::uint8_t>(Instruction::POP8):
            case static_cast<std::uint8_t>(Instruction::POP16):
            case static_cast<std::uint8_t>(Instruction::POP32):
            case static_cast<std::uint8_t>(Instruction::POP64):
            case static_cast<std::uint8_t>(Instruction::CALL):
                result << "r" << std::dec << static_cast<uint16_t>((step & 0b1111'0000) >> 4);
                break;
            default: {
                std::vector<std::uint16_t> operands;
                operands.push_back(static_cast<uint16_t>((step & 0b1111'0000) >> 4));
                operands.push_back(static_cast<uint16_t>(step & 0b0000'1111));
                result << "r" << operands[0] << ",r" << operands[1];
                break;
            }
        }
        result << std::endl;
    }
    for (const auto& section : this->optional_sections) {
        result << "section \"" << section->name << "\""
               << " datasize:" << section->datasize << std::endl;
    }
    return result.str();
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
