#include "tcp_connection.hpp"

#include <algorithm>
#include <iostream>

namespace attender
{
//#####################################################################################################################
    tcp_connection::tcp_connection(boost::asio::ip::tcp::socket socket)
        : socket_{std::move(socket)}
        , buffer_(config::buffer_size)
        , write_buffer_{}
        , read_callback_inst_{}
        , bytes_ready_{0}
        , kept_alive_{nullptr}
    {

    }
//---------------------------------------------------------------------------------------------------------------------
    tcp_connection::~tcp_connection()
    {
        std::cout << "connection died\n";
    }
//---------------------------------------------------------------------------------------------------------------------
    void tcp_connection::start()
    {
        // do_read();
    }
//---------------------------------------------------------------------------------------------------------------------
    void tcp_connection::set_read_callback(read_callback const& new_read_callback)
    {
        read_callback_inst_ = new_read_callback;
    }
//---------------------------------------------------------------------------------------------------------------------
    void tcp_connection::read()
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
                bytes_ready_ = bytes_transferred;
                read_callback_inst_(ec);
            }
        );
    }
//---------------------------------------------------------------------------------------------------------------------
    void tcp_connection::write(std::istream& stream, write_callback const& handler)
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
                            handler(ec);
                    }
                    else
                        handler(ec);
                }
            );
        }
        else
        {
            handler({});
        }
    }
//---------------------------------------------------------------------------------------------------------------------
    void tcp_connection::write(std::string const& string, write_callback const& handler)
    {
        write(string.c_str(), string.size(), handler);
    }
//---------------------------------------------------------------------------------------------------------------------
    void tcp_connection::write(char const* cstr, std::size_t count, write_callback const& handler)
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
    std::vector <char>& tcp_connection::get_read_buffer()
    {
        return buffer_;
    }
//---------------------------------------------------------------------------------------------------------------------
    std::size_t tcp_connection::ready_count() const
    {
        return bytes_ready_;
    }
//---------------------------------------------------------------------------------------------------------------------
    void tcp_connection::attach_lifetime_binder(lifetime_binder* ltb)
    {
        kept_alive_ = std::unique_ptr <lifetime_binder> (ltb);
    }
//#####################################################################################################################
    tcp_stream_device::tcp_stream_device(tcp_connection* connection)
        : connection_{connection}
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
//#####################################################################################################################
}
