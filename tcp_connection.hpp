#pragma once

#include "net_core.hpp"
#include "tcp_fwd.hpp"
#include "tcp_connection_interface.hpp"
#include "lifetime_binding.hpp"

#include <memory>
#include <utility>
#include <functional>
#include <mutex>

#include <iosfwd>
#include <boost/iostreams/categories.hpp>
#include <boost/asio/deadline_timer.hpp>

namespace attender
{
    class tcp_connection : public std::enable_shared_from_this <tcp_connection>
                         , public tcp_connection_interface
    {
        friend tcp_server;
        friend tcp_stream_device;

    public:
        using buffer_iterator = std::vector <char>::const_iterator;
        using lifetime_binder = lifetime_binding <request_handler, response_handler>;

    public:
        explicit tcp_connection(tcp_server_interface* parent, boost::asio::ip::tcp::socket socket);

        ~tcp_connection();

        /**
         *  Starts asynchronous reading.
         */
        void start();

        /**
         *  Closes the socket and aborts all messaging
         *  Also removes the livetime bindings
         */
        void stop();

        /**
         *  Gracefully shuts the socket down.
         */
        void shutdown();

        /**
         *  This function writes all the bytes from begin to end onto the stream.
         *  The handler function is called when the write operation completes.
         *  Do not (!) call write while another write operation is in progress!
         */
        template <typename IteratorT>
        void write(IteratorT&& begin, IteratorT&& end, write_callback handler)
        {
            write_buffer_.resize(end - begin);
            std::copy(begin, end, write_buffer_.begin());

            auto self{shared_from_this()};

            boost::asio::async_write(socket_, boost::asio::buffer(write_buffer_),
                [this, self, handler](boost::system::error_code ec, std::size_t)
                {
                    handler(ec);
                }
            );
        }

        /**
         *  This function writes the whole container onto the stream.
         *  The handler function is called when the write operation completes.
         *  Do not (!) call write while another write operation is in progress!
         */
        void write(std::vector <char> const& container, write_callback handler) override;

        /**
         *  This function writes the istream onto the tcp stream.
         *  The handler function is called when the write operation completes.
         *  Do not (!) call write while another write operation is in progress!
         */
        void write(std::istream& stream, write_callback handler) override;

        /**
         *  This function writes the string onto the tcp stream.
         *  The handler function is called when the write operation completes.
         *  Do not (!) call write while another write operation is in progress!
         */
        void write(std::string const& string, write_callback handler) override;

        /**
         *  Sets the read callback, which is called when a read operation finishes.
         */
        void set_read_callback(read_callback const& new_read_callback);

        /**
         *  Read more bytes into the buffer. This will overwrite the buffer.
         *
         *  Do not call this function while another read operation is in progress.
         *  This will lead into library-undefined behaviour.
         */
        void read();

        /**
         *  Returns the amount of bytes that remain in the read buffer.
         *
         *  @return A number of bytes that can be taken from the buffer.
         */
        std::size_t ready_count() const;

        /**
         *  Returns the beginning of the read buffer.
         *
         *  @return An iterator to the read buffer.
         */
        buffer_iterator begin() const;

        /**
         *  Returns the end of the read buffer.
         *
         *  @return An iterator to the read buffer.
         */
        buffer_iterator end() const;

        /**
         *  Returns the read buffer.
         *  Used in request_handler class.
         */
        std::vector <char>& get_read_buffer();

        /**
         *  Returns the tcp server behind this tcp connection.
         *  Do not abuse.
         */
        tcp_server_interface* get_parent() override;

        /**
         *  Return the associated socket.
         */
        asio::ip::tcp::socket* get_socket();

    private:
        void do_read();

        void attach_lifetime_binder(lifetime_binder* ltb);

        /**
         *  Checks the deadline_timer and possibly terminates the connection if the timeout was reached.
         *  Any IO will reset the deadline
         */
        void check_deadline(boost::asio::deadline_timer* timer, boost::system::error_code const& ec);

        /**
         *  Returns wether the socket is open or not.
         */
        bool stopped() const;

    private:
        tcp_server_interface* parent_;
        asio::ip::tcp::socket socket_;
        std::vector <char> buffer_;
        std::vector <char> write_buffer_;
        read_callback read_callback_inst_;
        std::size_t bytes_ready_;
        boost::asio::deadline_timer read_timeout_timer_;

        std::unique_ptr <lifetime_binder> kept_alive_;
    };

    class tcp_stream_device
    {
    public:
        using char_type = char;
        using category = boost::iostreams::source_tag;

        tcp_stream_device(tcp_connection* connection);

        std::streamsize read(char_type* s, std::streamsize n);

    private:
        tcp_connection* connection_;
        std::size_t pos_;
    };
}
