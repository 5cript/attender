#include "tcp_connection.hpp"
#include "debug.hpp"

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
        , read_timeout_timer_{socket_.get_io_service()}
        , kept_alive_{nullptr}
    {
        // this will ensure, that the timer does not fire right away when it it started
        // the timer is started on read, but the timeout will be set later in every do_read cycle.
        read_timeout_timer_.expires_at(boost::posix_time::pos_infin);
    }
//---------------------------------------------------------------------------------------------------------------------
    tcp_connection::~tcp_connection()
    {
        if (!stopped())
            stop();
        std::cout << "connection died\n";

        // This must be the last action of this function
        kept_alive_.reset(nullptr);
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
        read_timeout_timer_.async_wait(
            [this](boost::system::error_code const& ec)
            {
                check_deadline(&read_timeout_timer_, ec);
            }
        );


        do_read();
    }
//---------------------------------------------------------------------------------------------------------------------
    void tcp_connection::stop()
    {
        socket_.close();
        read_timeout_timer_.cancel();
    }
//---------------------------------------------------------------------------------------------------------------------
    void tcp_connection::shutdown()
    {
        // this must be called before stop.
        boost::system::error_code ignored_ec;
        socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
    }
//---------------------------------------------------------------------------------------------------------------------
    void tcp_connection::do_read()
    {
        // sets / resets the timer.
        read_timeout_timer_.expires_from_now(boost::posix_time::seconds(config::read_timeout));

        socket_.async_read_some(boost::asio::buffer(buffer_),
            [this](boost::system::error_code ec, std::size_t bytes_transferred)
            {
                bytes_ready_ = bytes_transferred;
                read_timeout_timer_.expires_from_now(boost::posix_time::pos_infin);
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
                [this, cb{handler}, &stream](boost::system::error_code ec, std::size_t)
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
    void tcp_connection::attach_lifetime_binder(lifetime_binding* ltb)
    {
        kept_alive_ = std::unique_ptr <lifetime_binding> (ltb);
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
//---------------------------------------------------------------------------------------------------------------------
    std::string tcp_connection::get_remote_address() const
    {
        boost::system::error_code ec;
        auto endpoint = socket_.remote_endpoint(ec);

        if (ec)
            return "";

        return endpoint.address().to_string();
    }
//---------------------------------------------------------------------------------------------------------------------
    unsigned short tcp_connection::get_remote_port() const
    {
        boost::system::error_code ec;
        auto endpoint = socket_.remote_endpoint(ec);

        if (ec)
            return 0;

        return endpoint.port();
    }
//---------------------------------------------------------------------------------------------------------------------
    bool tcp_connection::stopped() const
    {
        return !socket_.is_open();
    }
//---------------------------------------------------------------------------------------------------------------------
    void tcp_connection::check_deadline(boost::asio::deadline_timer* timer, boost::system::error_code const& ec)
    {
        // The socket is not open anymore, therefore timeout checkings are no longer relevant.
        if (stopped())
            return;

        //if (ec == boost::system::errc::operation_canceled)
        //    return;

        if (ec && ec != boost::system::errc::operation_canceled)
        {
            // There has been an error, no matter what, close the connection.
            DUMP(ec, ATTENDER_CODE_PLACE);

            shutdown();
            stop();
            return;
        }

        // Check whether the deadline has passed. We compare the deadline against
        // the current time since a new asynchronous operation may have moved the
        // deadline before this actor had a chance to run.
        if (timer->expires_at() <= boost::asio::deadline_timer::traits_type::now())
        {
            shutdown();
            stop();
        }
        else
        {
            timer->async_wait(
                [this, timer](boost::system::error_code const& ec)
                {
                    check_deadline(timer, ec);
                }
            );
        }
    }
//#####################################################################################################################
    tcp_stream_device::tcp_stream_device(tcp_connection_interface* connection)
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
