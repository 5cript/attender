#pragma once

#include <cstddef>
#include <boost/asio.hpp>

// these macros are meant to be overridden by -D flag
// do not define them anywhere else.

#ifndef CONFIG_MAX_HEADER_FIELDS
#   define CONFIG_MAX_HEADER_FIELDS 1024
#endif // CONFIG_MAX_HEADER_FIELDS

#ifndef CONFIG_MAX_HEADER_BUFFER
#   define CONFIG_MAX_HEADER_BUFFER 16384
#endif // CONFIG_MAX_HEADER_BUFFER

#ifndef CONFIG_RECEIVE_BUFFER_SIZE
#   define CONFIG_RECEIVE_BUFFER_SIZE 4096
#endif // CONFIG_RECEIVE_BUFFER_SIZE

namespace attender
{
    namespace asio = boost::asio;

    namespace config
    {
        constexpr static std::size_t buffer_size = CONFIG_RECEIVE_BUFFER_SIZE;
        constexpr static std::size_t header_buffer_max = CONFIG_MAX_HEADER_BUFFER;
        constexpr static std::size_t header_field_max = CONFIG_MAX_HEADER_FIELDS;
    }
}
