
#include "utils.hpp"

#include <boost/filesystem.hpp>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#ifdef __linux__
#    include <sys/stat.h>
#endif
#include <thread>

#include "container_printer.hpp"
#include "logging.hpp"
namespace nyulan {
namespace utils {
namespace fsys = boost::filesystem;
Endian check_native_endian() {
    std::uint16_t bom = 0x1100;
    Endian result;
    switch (reinterpret_cast<std::uint8_t *>(&bom)[0]) {
        case 0x00:
            result = Endian::LITTLE;
            break;
        case 0x11:
            result = Endian::BIG;
            break;
    }
    return result;
}
std::uint8_t ascii_printable(std::uint8_t character) {
    if (' ' <= character && character <= '~') {
        return character;
    } else {
        return '.';
    }
}
void hexdump_system(std::vector<std::uint8_t> &data, std::string options) {
    using namespace nyulan::container_ostream;
    auto tmp_dir = fsys::temp_directory_path();
    auto tmp_to_hex_filename = fsys::unique_path("nyulan_hexdump_%%%%-%%%%-%%%%-%%%%");
    auto tmp_from_hex_filename = fsys::unique_path("nyulan_hexdump_%%%%-%%%%-%%%%-%%%%");
    auto tmp_to_hex_filepath = tmp_dir / tmp_to_hex_filename;
    TRIVIAL_LOG_WITH_FUNCNAME(debug) << "to_hex:" << tmp_to_hex_filepath;
    auto tmp_from_hex_filepath = tmp_dir / tmp_from_hex_filename;
    TRIVIAL_LOG_WITH_FUNCNAME(debug) << "from_hex" << tmp_from_hex_filepath;
    if (mkfifo(tmp_to_hex_filepath.c_str(), static_cast<int>(fsys::perms::owner_write | fsys::perms::owner_read)) ==
        -1) {
        throw std::runtime_error("failed to create fifo " + std::string(tmp_to_hex_filepath.c_str()));
    }
    if (mkfifo(tmp_from_hex_filepath.c_str(), static_cast<int>(fsys::perms::owner_read | fsys::perms::owner_write)) ==
        -1) {
        throw std::runtime_error("failed to create fifo " + std::string(tmp_from_hex_filepath.c_str()));
    }
    std::stringstream command;
    command << "hexdump " << options << " " << tmp_to_hex_filepath.c_str() << " >" << tmp_from_hex_filepath.c_str();
    std::stringstream data_strrized;
    data_strrized << data;
    TRIVIAL_LOG_WITH_FUNCNAME(debug) << data_strrized.str() << std::endl;
    // write to hexdump
    std::thread write_data([&tmp_to_hex_filepath, &data] {
        std::basic_ofstream<std::uint8_t> to_hex(tmp_to_hex_filepath, std::ios_base::binary);
        if (not to_hex.is_open()) {
            throw std::runtime_error("failed to open file " + std::string(tmp_to_hex_filepath.c_str()));
        }
        to_hex.write(data.data(), data.size());
        to_hex.close();
    });
    // call hexdump
    std::thread invoke_hexdump([&command] {
        TRIVIAL_LOG_WITH_FUNCNAME(debug) << command.str();
        auto exit_status = system(command.str().c_str());
        if (exit_status != 0) {
            throw std::runtime_error("command exitted with code " + std::to_string(exit_status));
        }
    });
    // read from hexdump
    std::thread print_result([&tmp_from_hex_filepath] {
        std::ifstream from_hex(tmp_from_hex_filepath.c_str());
        if (not from_hex.is_open()) {
            throw std::runtime_error("failed to open file " + std::string(tmp_from_hex_filepath.c_str()));
        }
        std::string buf_line;
        while (not from_hex.eof()) {
            std::getline(from_hex, buf_line);
            std::cout << buf_line << std::endl;
            buf_line.clear();
        }
        from_hex.close();
    });
    invoke_hexdump.join();
    print_result.join();
    write_data.join();
    fsys::remove(tmp_to_hex_filepath);
    fsys::remove(tmp_from_hex_filepath);
}
void hexdump_simple(std::vector<std::uint8_t> &data, std::string) {
    auto itr_data = data.begin();
    for (size_t idx_full = 0; idx_full < data.size() / 16; idx_full++) {
        std::cout << std::hex << std::setw(8) << std::setfill('0') << std::distance(data.begin(), itr_data) << "  ";
        for (auto i = 0; i < 16; i++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<std::uint16_t>(*itr_data)
                      << ((i % 8 == 7) ? "  " : " ");
            itr_data++;
        }
        itr_data -= 16;  // NOTE: もう一度読み込むために戻す
        std::cout << "|";
        for (auto i = 0; i < 16; i++) {
            std::cout << ascii_printable(*itr_data);
            itr_data++;
        }
        std::cout << "|" << std::endl;
    }
    //余り物
    auto remainder = data.size() % 16;
    std::cout << std::hex << std::setw(8) << std::setfill('0') << std::distance(data.begin(), itr_data) << "  ";
    for (size_t i = 0; i < 16; i++) {
        if (i < remainder) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<std::uint16_t>(*itr_data)
                      << ((i % 8 == 7) ? "  " : " ");
            itr_data++;
        } else {
            std::cout << "  " << ((i % 8 == 7) ? "  " : " ");  //埋め草
        }
    }
    itr_data -= remainder;  // NOTE: もう一度読み込むために戻す
    std::cout << "|";
    for (size_t i = 0; i < remainder; i++) {
        std::cout << ascii_printable(*itr_data);
        itr_data++;
    }
    std::cout << "|" << std::endl;
}
void hexdump(std::vector<std::uint8_t> &data, std::string options) {
    // hexdump_system(data,options);
    hexdump_simple(data, options);
}
}  // namespace utils
}  // namespace nyulan
