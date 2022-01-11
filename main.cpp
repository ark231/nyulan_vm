
#include <boost/program_options.hpp>
#include <fstream>
#include <iostream>
#include <string>

#include "objectfile.hpp"
#include "vm.hpp"
namespace bpo = boost::program_options;

int main(int argc, char **argv) {
    bpo::options_description opt("option");
    opt.add_options()("help,h", "show this help")("objectfile,s", bpo::value<std::string>(), "object file");

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
    nyulan::ObjectFile objectfile;
    try {
        objectfile = nyulan::ObjectFile(varmap["objectfile"].as<std::string>());
    } catch (std::exception *except) {
        std::cerr << "failed to read objectfile" << std::endl;
        std::cerr << except->what() << std::endl;
        exit(EXIT_FAILURE);
    }
    nyulan::VirtualMachine VM(objectfile.literal_datas);
    return 0;
}
