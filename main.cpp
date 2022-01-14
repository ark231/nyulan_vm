
#include <boost/program_options.hpp>
#include <boost/stacktrace.hpp>
#include <fstream>
#include <iostream>
#include <string>

#include "objectfile.hpp"
#include "vm.hpp"
namespace bpo = boost::program_options;

#ifndef NDEBUG
[[noreturn]] void terminate_with_stacktrace() noexcept;
#endif

int main(int argc, char **argv) {
#ifndef NDEBUG
    std::set_terminate(terminate_with_stacktrace);
#endif
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
    VM.exec(objectfile.code);
    return 0;
}
#ifndef NDEBUG
[[noreturn]] void terminate_with_stacktrace() noexcept {
    std::cerr << "terminate() has called!" << std::endl;
    std::cerr << "---- stacktrace ----" << std::endl;
    std::cerr << boost::stacktrace::stacktrace() << std::endl;
    std::cerr << "---- stacktrace end----" << std::endl << std::endl;
    std::set_terminate(nullptr);
    std::terminate();
    std::abort();
}
#endif
