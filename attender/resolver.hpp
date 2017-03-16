#pragma once

#include <boost/asio.hpp>

namespace attender {
    using tcp_resolver = boost::asio::ip::tcp::resolver;
    using icmp_resolver = boost::asio::ip::icmp::resolver;
}
