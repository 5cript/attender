#pragma once

#include "net_core.hpp"
#include "tcp_fwd.hpp"
#include "tcp_connection_interface.hpp"

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

    public:
        explicit tcp_connection(boost::asio::ip::tcp::socket socket,
                                read_handler read_handler_inst);

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
        void write(IteratorT const& begin, IteratorT const& end, write_handler const& handler)
        {
            write_buffer_.resize(end - begin);
            std::copy(begin, end, write_buffer_.begin());

            auto self{shared_from_this()};

            boost::asio::async_write(socket_, boost::asio::buffer(write_buffer_),
                [this, self, &handler](boost::system::error_code ec, std::size_t)
                {
                    handler(ec, self);
                }
            );
        }

        /**
         *  This function writes the whole container onto the stream.
         *  The handler function is called when the write operation completes.
         *  Do not (!) call write while another write operation is in progress!
         */
        template <template <typename T, typename> class ContainerT, typename Allocator = std::allocator <char> >
        void write(ContainerT <char, Allocator> const& container, write_handler const& handler)
        {
            write(std::begin(container), std::end(container), handler);
        }

        /**
         *  This function writes the istream onto the tcp stream.
         *  The handler function is called when the write operation completes.
         *  Do not (!) call write while another write operation is in progress!
         */
        void write(std::istream& stream, write_handler const& handler);

        /**
         *  This function writes the char buffer onto the tcp stream.
         *  The handler function is called when the write operation completes.
         *  Do not (!) call write while another write operation is in progress!
         */
        void write(char const* cstr, std::size_t count, write_handler const& handler);

    private:
        void do_read();

        buffer_iterator begin() const;
        buffer_iterator end() const;

        std::size_t ready_count() const noexcept;

        asio::ip::tcp::socket socket_;
        std::vector <char> buffer_;
        std::vector <char> write_buffer_;
        read_handler read_handler_inst_;
        std::size_t bytes_ready_;
    };

    class tcp_stream_device
    {
        friend tcp_connection;

    public:
        using char_type = char;
        using category = boost::iostreams::source_tag;

        std::streamsize read(char_type* s, std::streamsize n);

        /**
         *  did you encounter an EOF?
         *  call read_more to issue a new read cycle to get more bytes from the socket.
         */
        void read_from_socket();

        /**
         *  Returns the shared tcp_connection object.
         */
        std::shared_ptr <tcp_connection> get_connection() const;

    private:
        tcp_stream_device(std::shared_ptr <tcp_connection> connection);

        std::shared_ptr <tcp_connection> connection_;
        std::size_t pos_;
    };
}
