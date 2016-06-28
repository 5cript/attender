#include "tcp_connection.hpp"

#include <algorithm>
#include <iostream>

namespace attender
{
//#####################################################################################################################
    tcp_connection::tcp_connection(tcp_server_interface* parent, boost::asio::ip::tcp::socket socket)
        : parent_(parent)
        , socket_{std::move(socket)}
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
        kept_alive_.reset(nullptr);
    }
//---------------------------------------------------------------------------------------------------------------------
    void tcp_connection::shutdown()
    {
        boost::system::error_code ignored_ec;
        socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
    }
//---------------------------------------------------------------------------------------------------------------------
    void tcp_connection::do_read()
    {
        socket_.async_read_some(boost::asio::buffer(buffer_),
            [this, self{shared_from_this()}](boost::system::error_code ec, std::size_t bytes_transferred)
            {
                bytes_ready_ = bytes_transferred;
                read_callback_inst_(ec);
            }
        );
    }
//---------------------------------------------------------------------------------------------------------------------
    void tcp_connection::write(std::istream& stream, write_callback handler)
    {
        write_buffer_.resize(config::buffer_size);
        stream.read(write_buffer_.data(), config::buffer_size);

        if (stream.gcount() != 0)
        {
            boost::asio::async_write(socket_, boost::asio::buffer(write_buffer_),
                [this, self{shared_from_this()}, cb{handler}, &stream](boost::system::error_code ec, std::size_t)
                {
                    if (!ec)
                    {
                        if (stream.gcount() == config::buffer_size)
                            write(stream, cb);
                        else
                            cb(ec);
                    }
                    else
                        cb(ec);
                }
            );
        }
        else
        {
            handler({});
        }
    }
//---------------------------------------------------------------------------------------------------------------------
    void tcp_connection::write(std::vector <char> const& container, write_callback handler)
    {
        write(std::begin(container), std::end(container), handler);
    }
//---------------------------------------------------------------------------------------------------------------------
    void tcp_connection::write(std::string const& string, write_callback handler)
    {
        write(std::begin(string), std::end(string), handler);
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
//---------------------------------------------------------------------------------------------------------------------
    tcp_server_interface* tcp_connection::get_parent()
    {
        return parent_;
    }
//---------------------------------------------------------------------------------------------------------------------
    asio::ip::tcp::socket* tcp_connection::get_socket()
    {
        return &socket_;
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
