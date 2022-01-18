#ifndef NYULAN_DEBUGLOG
#define NYULAN_DEBUGLOG

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <filesystem>
#define _filename_ std::filesystem::path(__FILE__).filename()  //__FILENAME__にしたいけど、予約されてるから、、、
#define TRIVIAL_LOG_WITH_FUNCNAME(severage) \
    BOOST_LOG_TRIVIAL(severage) << _filename_ << ":" << __LINE__ << " @" << __func__ << "() "
namespace nyulan {
namespace logging {
void init(const boost::log::trivial::severity_level severity);
}
}  // namespace nyulan

#endif
