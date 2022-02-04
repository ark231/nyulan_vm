
#include <boost/program_options.hpp>
#include <boost/stacktrace.hpp>
#include <cassert>
#include <filesystem>
#include <iostream>
#include <limits>
#include <magic_enum.hpp>
#include <sstream>
#include <thread>
#include <tuple>

#include "container_printer.hpp"
#include "logging.hpp"
#include "objectfile.hpp"
#include "vm.hpp"
namespace bpo = boost::program_options;
namespace stdfsys = std::filesystem;
[[noreturn]] void terminate_with_stacktrace() noexcept;
enum class OutputType {
    ADDR2LINE,
    ADDR2SRC,
};
namespace ndb {
std::string getline_at(std::istream *, std::uint64_t line);
namespace debugSection {
using Section = std::shared_ptr<nyulan::ObjectFile::OptionalSection>;
struct FilenumAndLine {
    std::uint64_t filenum;
    std::uint64_t line;

    FilenumAndLine(std::uint64_t filenum, std::uint64_t line) : filenum(filenum), line(line) {}
};
FilenumAndLine addr2line(Section section, nyulan::Address step_address);
std::vector<std::string> filenames(Section section);
std::uint64_t bytes_to_value(Section section, std::vector<std::uint8_t> data, size_t bitwidth);
}  // namespace debugSection

namespace callbacks {
namespace addr2line {
class OnLoopHead {
    nyulan::ObjectFile &objectfile_;
    debugSection::Section section_addr2line_;
    debugSection::Section section_srcfiles_;

   public:
    OnLoopHead(nyulan::ObjectFile &objectfile) : objectfile_(objectfile) {
        section_addr2line_ = (objectfile_.find_section("debug.addr2line"));
        section_srcfiles_ = (objectfile_.find_section("debug.sourcefiles"));
    }
    void operator()(nyulan::Address addr, nyulan::OneStep) {
        auto filenum = ndb::debugSection::addr2line(section_addr2line_, addr).filenum;
        auto line = ndb::debugSection::addr2line(section_addr2line_, addr).line;
        auto filenames = ndb::debugSection::filenames(section_srcfiles_);
        stdfsys::path filepath(filenames[filenum]);
        std::cout << "@" << filepath.filename() << " line: " << line << std::endl;
    }
};
}  // namespace addr2line
namespace addr2src {
class OnLoopHead {
    nyulan::ObjectFile &objectfile_;
    debugSection::Section section_addr2line_;
    debugSection::Section section_srcfiles_;
    std::vector<std::shared_ptr<std::ifstream>> sourcefiles_;

   public:
    OnLoopHead(nyulan::ObjectFile &objectfile) : objectfile_(objectfile) {
        section_addr2line_ = (objectfile_.find_section("debug.addr2line"));
        section_srcfiles_ = (objectfile_.find_section("debug.sourcefiles"));
        for (const auto &filename : ndb::debugSection::filenames(section_srcfiles_)) {
            TRIVIAL_LOG_WITH_FUNCNAME(debug) << filename;
            sourcefiles_.emplace_back(new std::ifstream(filename));
            TRIVIAL_LOG_WITH_FUNCNAME(debug) << std::boolalpha << sourcefiles_.back()->is_open();
        }
    }
    void operator()(nyulan::Address addr, nyulan::OneStep) {
        auto filenum_and_line = ndb::debugSection::addr2line(section_addr2line_, addr);
        auto filenum = filenum_and_line.filenum;
        auto line = filenum_and_line.line;
        auto sourcefile = sourcefiles_.at(filenum).get();
        if (not sourcefile->is_open()) {
            std::cout << "filenum:" << filenum << "line:" << line << std::endl;
            std::cerr << "warn: failed to open sourcefile, causing alternate output to be enabled." << std::endl;
        } else {
            std::cout << ndb::getline_at(sourcefile, line) << std::endl;
        }
    }
};
}  // namespace addr2src
}  // namespace callbacks

}  // namespace ndb
using namespace nyulan::container_ostream;
int main(int argc, char **argv) {
    std::set_terminate(terminate_with_stacktrace);
    bpo::options_description opt("option");
    std::stringstream output_type_description;
    output_type_description << "type of output " << magic_enum::enum_names<OutputType>();
    opt.add_options()("help,h", "show this help")("objectfile,s", bpo::value<std::string>(), "object file")(
        "output-type,t", bpo::value<std::string>(), output_type_description.str().c_str())(
        "debug,d", "enable features for debugging this debugger");

    bpo::variables_map varmap;
    bpo::store(bpo::parse_command_line(argc, argv, opt), varmap);
    bpo::notify(varmap);
    if (varmap.count("help")) {
        std::cout << opt << std::endl;
        std::exit(EXIT_SUCCESS);
    }
    if (!varmap.count("objectfile")) {
        std::cerr << "error: no objectfile speciried" << std::endl;
        std::cout << opt << std::endl;
        std::exit(EXIT_FAILURE);
    }
    if (varmap.count("debug")) {
        nyulan::logging::init(boost::log::trivial::debug);
    } else {
        nyulan::logging::init(boost::log::trivial::info);
    }
    nyulan::ObjectFile objectfile;
    try {
        objectfile = nyulan::ObjectFile(varmap["objectfile"].as<std::string>());
    } catch (std::exception *except) {
        std::cerr << "failed to read objectfile" << std::endl;
        std::cerr << except->what() << std::endl;
        exit(EXIT_FAILURE);
    }
    if (!varmap.count("output-type")) {
        std::cerr << "missing output type" << std::endl;
        std::cerr << opt << std::endl;
        exit(EXIT_FAILURE);
    }
    auto output_type = magic_enum::enum_cast<OutputType>(varmap["output-type"].as<std::string>());
    if (!output_type) {
        std::cerr << "can't understand output type \"" << varmap["output-type"].as<std::string>() << "\"";
        std::cerr << opt << std::endl;
        exit(EXIT_FAILURE);
    }
    nyulan::CallBacks debug_outputs;
    switch (output_type.value()) {
        case OutputType::ADDR2LINE:
            debug_outputs.on_loop_head = ndb::callbacks::addr2line::OnLoopHead(objectfile);
            break;
        case OutputType::ADDR2SRC:
            debug_outputs.on_loop_head = std::move(ndb::callbacks::addr2src::OnLoopHead(objectfile));
            break;
        default:
            std::cerr << "currently not supported" << std::endl;
            exit(EXIT_FAILURE);
    }
    nyulan::VirtualMachine VM(objectfile.literal_datas, true);
    VM.install_callbacks(debug_outputs);
    VM.exec(objectfile.code, objectfile.find_label("_start").label_address);
    return 0;
}
[[noreturn]] void terminate_with_stacktrace() noexcept {
    std::cerr << "terminate() has called!" << std::endl;
    std::cerr << "---- stacktrace ----" << std::endl;
    std::cerr << boost::stacktrace::stacktrace() << std::endl;
    std::cerr << "---- stacktrace end----" << std::endl << std::endl;
    std::set_terminate(nullptr);
    std::terminate();
    std::abort();
}
namespace ndb {
std::string getline_at(std::istream *stream, std::uint64_t line) {
    if (line == 0) {
        return "unknown";
    }
    stream->seekg(0);
    for (size_t i = 0; i < line - 1; i++) {
        stream->ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
    std::string result;
    std::getline(*stream, result);
    return result;
}
namespace debugSection {
FilenumAndLine addr2line(Section section, nyulan::Address step_address) {
    if (section->name != "debug.addr2line") {
        throw std::invalid_argument("addr2line() can only parse \"debug.addr2line\" section");
    }
    auto src_bitwidth = section->data[0];
    auto line_bitwidth = section->data[1];
    auto data_bitwidth = src_bitwidth + line_bitwidth;
    auto data_index = 2 + (data_bitwidth / 8) * step_address.value();
    auto src_begin = section->data.begin() + data_index;
    auto src_end = src_begin + (src_bitwidth / 8);
    auto line_begin = src_end;
    auto line_end = line_begin + (line_bitwidth / 8);

    auto src_filenum = bytes_to_value(section, std::vector(src_begin, src_end), src_bitwidth);
    auto linenum = bytes_to_value(section, std::vector(line_begin, line_end), line_bitwidth);
    return FilenumAndLine(src_filenum, linenum);
}
std::uint64_t bytes_to_value(Section section, std::vector<std::uint8_t> data, size_t bitwidth) {
    switch (bitwidth) {
        case 8:
            return data[0];
        case 16:
            return section->parent->bytes_to_value<std::uint16_t>(data);
        case 32:
            return section->parent->bytes_to_value<std::uint32_t>(data);
        case 64:
            return section->parent->bytes_to_value<std::uint64_t>(data);
        default:
            throw std::invalid_argument("bitwidth of " + std::to_string(bitwidth) + " is too big");
    }
}
std::vector<std::string> filenames(Section section) {
    if (section->name != "debug.sourcefiles") {
        throw std::invalid_argument("filenames() can only parse \"debug.sourcefiles\" section");
    }
    std::vector<std::string> result;
    std::string tmp;
    for (const auto &datum : section->data) {
        if (datum != '\0') {
            tmp.push_back(datum);
        } else {
            result.push_back(tmp);
            tmp.clear();
        }
    }
    std::stringstream res;
    res << result;
    TRIVIAL_LOG_WITH_FUNCNAME(debug) << res.str();
    return result;
}

}  // namespace debugSection
}  // namespace ndb
