
#include <boost/program_options.hpp>
#include <iostream>

#include "logging.hpp"
#include "objectfile.hpp"
#include "utils.hpp"
namespace bpo = boost::program_options;
int main(int argc, char **argv) {
    bpo::options_description opt("option");
    opt.add_options()("help,h", "show this help")("objectfile,s", bpo::value<std::string>(), "object file")(
        "hexdump-sections,S", "dump sections' contents")("debug,d", "enable debug outputs");

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
    if (not varmap.count("hexdump-sections")) {
        std::cout << objectfile.pretty() << std::endl;
    } else {
        for (const auto &section : objectfile.optional_sections) {
            std::cout << "section \"" << section->name << "\" with datasize " << std::dec << section->datasize << ":"
                      << std::endl;
            nyulan::utils::hexdump(section->data);
        }
    }
    return 0;
}
