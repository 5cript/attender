#include "tcp_connection.hpp"

#include <algorithm>
#include <iostream>

namespace attender
{
//#####################################################################################################################
    tcp_connection::tcp_connection(boost::asio::ip::tcp::socket socket,
                                   read_handler read_handler_inst)
        : socket_{std::move(socket)}
        , buffer_(config::buffer_size)
        , write_buffer_{}
        , read_handler_inst_{std::move(read_handler_inst)}
        , bytes_ready_{0}
    {

    }
//---------------------------------------------------------------------------------------------------------------------
    void tcp_connection::start()
    {
        do_read();
    }
//---------------------------------------------------------------------------------------------------------------------
    void tcp_connection::stop()
    {
        socket_.close();
    }
//---------------------------------------------------------------------------------------------------------------------
    void tcp_connection::do_read()
    {
        auto self{shared_from_this()};
        socket_.async_read_some(boost::asio::buffer(buffer_),
            [this, self](boost::system::error_code ec, std::size_t bytes_transferred)
            {
                if (!ec)
                {
                    bytes_ready_ = bytes_transferred;
                    read_handler_inst_(tcp_stream_device{self});
                }
            }
        );
    }
//---------------------------------------------------------------------------------------------------------------------
    void tcp_connection::write(std::istream& stream, write_handler const& handler)
    {
        auto self{shared_from_this()};

        write_buffer_.resize(config::buffer_size);
        stream.read(write_buffer_.data(), config::buffer_size);

        if (stream.gcount() != 0)
        {
            boost::asio::async_write(socket_, boost::asio::buffer(write_buffer_),
                [this, self, handler, &stream](boost::system::error_code ec, std::size_t)
                {
                    if (!ec)
                    {
                        if (stream.gcount() == config::buffer_size)
                            write(stream, handler);
                        else
                            handler(ec, self);
                    }
                    else
                        handler(ec, self);
                }
            );
        }
        else
        {
            handler({}, self);
        }
    }
//---------------------------------------------------------------------------------------------------------------------
    void tcp_connection::write(char const* cstr, std::size_t count, write_handler const& handler)
    {
        write(cstr, cstr + count, handler);
    }
//---------------------------------------------------------------------------------------------------------------------
    tcp_connection::buffer_iterator tcp_connection::begin() const
    {
        return std::cbegin(buffer_);
    }
//---------------------------------------------------------------------------------------------------------------------
    tcp_connection::buffer_iterator tcp_connection::end() const
    {
        return std::cbegin(buffer_) + bytes_ready_;
    }
//---------------------------------------------------------------------------------------------------------------------
    std::size_t tcp_connection::ready_count() const noexcept
    {
        return bytes_ready_;
    }
//#####################################################################################################################
    tcp_stream_device::tcp_stream_device(std::shared_ptr <tcp_connection> connection)
        : connection_{std::move(connection)}
        , pos_{0}
    {
    }
//---------------------------------------------------------------------------------------------------------------------
    std::streamsize tcp_stream_device::read(char_type* s, std::streamsize n)
    {
        std::streamsize amt = static_cast <std::streamsize> (connection_->ready_count() - pos_);
        std::streamsize to_read = std::min(n, amt);
        if (to_read != 0)
        {
            std::copy(connection_->begin() + pos_, connection_->begin() + pos_ + to_read, s);
            pos_ += to_read;
            return to_read;
        }

        return -1; // EOF - no more data to read
    }
//---------------------------------------------------------------------------------------------------------------------
    void tcp_stream_device::read_from_socket()
    {
        connection_->do_read();
    }
//---------------------------------------------------------------------------------------------------------------------
    std::shared_ptr <tcp_connection> tcp_stream_device::get_connection() const
    {
        return connection_;
    }
//#####################################################################################################################
}
