#pragma once

#include <boost/exception/all.hpp>

typedef boost::error_info<struct tag_message, std::string> error_message;

struct basic_error : virtual boost::exception, virtual std::exception {};
struct flow_exit : virtual boost::exception, virtual std::exception {};