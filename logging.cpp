
#include "logging.hpp"

#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
namespace nyulan {
namespace logging {
void init(const boost::log::trivial::severity_level severity) {
    boost::log::core::get()->set_filter(boost::log::trivial::severity >= severity);
    boost::log::add_common_attributes();
    boost::log::add_console_log(std::cout, boost::log::keywords::format = "[%TimeStamp%] :%Message%");
}
}  // namespace logging
}  // namespace nyulan
