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
        explicit tcp_connection(boost::asio::ip::tcp::socket socket);

        ~tcp_connection();

        /**
         *  Starts asynchronous reading.
         */
        void start();

        /**
         *  Closes the socket and aborts all messaging
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
        void write(IteratorT const& begin, IteratorT const& end, write_callback const& handler)
        {
            write_buffer_.resize(end - begin);
            std::copy(begin, end, write_buffer_.begin());

            auto self{shared_from_this()};

            boost::asio::async_write(socket_, boost::asio::buffer(write_buffer_),
                [this, self, &handler](boost::system::error_code ec, std::size_t)
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
        template <template <typename T, typename> class ContainerT, typename Allocator = std::allocator <char> >
        void write(ContainerT <char, Allocator> const& container, write_callback const& handler)
        {
            write(std::begin(container), std::end(container), handler);
        }

        /**
         *  This function writes the istream onto the tcp stream.
         *  The handler function is called when the write operation completes.
         *  Do not (!) call write while another write operation is in progress!
         */
        void write(std::istream& stream, write_callback const& handler);

        /**
         *  This function writes the string onto the tcp stream.
         *  The handler function is called when the write operation completes.
         *  Do not (!) call write while another write operation is in progress!
         */
        void write(std::string const& string, write_callback const& handler);

        /**
         *  This function writes the char buffer onto the tcp stream.
         *  The handler function is called when the write operation completes.
         *  Do not (!) call write while another write operation is in progress!
         */
        void write(char const* cstr, std::size_t count, write_callback const& handler);

        /**
         *  Sets the read callback, which is called when a read operation finishes.
         */
        void set_read_callback(read_callback const& new_read_callback);

        /**
         *  Read more bytes into the buffer. This will overwrite the buffer.
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

        std::vector <char>& get_read_buffer();

    private:
        void do_read();

        void attach_lifetime_binder(lifetime_binder* ltb);

    private:
        asio::ip::tcp::socket socket_;
        std::vector <char> buffer_;
        std::vector <char> write_buffer_;
        read_callback read_callback_inst_;
        std::size_t bytes_ready_;

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
