#pragma once

#include "tcp_fwd.hpp"

#include <boost/asio.hpp>

#include <iosfwd>
#include <string>
#include <vector>

namespace attender
{
    class tcp_connection_interface
    {
    public:
        using buffer_iterator = std::vector <char>::const_iterator;

        // control
        virtual void start() = 0;
        virtual void stop() = 0;
        virtual void shutdown() = 0;

        // reading
        virtual void read() = 0;

        // writing
        virtual void write(std::istream& stream, write_callback handler) = 0;
        virtual void write(std::string const& string, write_callback handler) = 0;
        virtual void write(std::vector <char> const& container, write_callback handler) = 0;
        virtual std::size_t ready_count() const = 0;

        // buffer
        virtual buffer_iterator begin() const = 0;
        virtual buffer_iterator end() const = 0;
        virtual std::vector <char>& get_read_buffer() = 0;

        // auxiliary info
        virtual std::string get_remote_address() const = 0;
        virtual unsigned short get_remote_port() const = 0;

        // internal control
        virtual tcp_server_interface* get_parent() = 0;
        virtual void set_read_callback(read_callback const& new_read_callback) = 0;
        virtual boost::asio::ip::tcp::socket* get_socket() = 0;

        // interface virtual destructor
        virtual ~tcp_connection_interface() = default;

        virtual response_handler& get_response_handler() = 0;
        virtual request_handler& get_request_handler() = 0;
    };
}
