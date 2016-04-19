#pragma once

#include <cstddef>
#include <boost/asio.hpp>

namespace attender
{
    namespace asio = boost::asio;

    namespace config
    {
        constexpr static std::size_t buffer_size = 10;
    }
}
