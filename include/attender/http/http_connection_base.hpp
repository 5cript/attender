#pragma once

#include <attender/net_core.hpp>
#include <attender/http/http_fwd.hpp>
#include <attender/http/http_connection_interface.hpp>
#include <attender/http/lifetime_binding.hpp>
#include <attender/utility/debug.hpp>

#include <boost/asio/deadline_timer.hpp>
#include <boost/iostreams/categories.hpp>

#include <algorithm>
#include <memory>
#include <utility>
#include <functional>
#include <mutex>
#include <atomic>
#include <utility>
#include <type_traits>

namespace attender
{
    namespace internal
    {
        template <typename T>
        auto get_socket_layer(T& sock) -> typename T::lowest_layer_type&
        {
            return sock.lowest_layer();
        }

        template <typename T, typename U = void>
        struct get_executor
        {
        };

        template <typename T>
        struct get_executor <T, std::enable_if_t <!std::is_same_v <T, boost::asio::ip::tcp::socket>>>
        {
            static auto ctx(T* sock) -> decltype(auto)
            {
                return sock->lowest_layer().get_executor();
            }
        };

        template <>
        struct get_executor <boost::asio::ip::tcp::socket>
        {
            using U = boost::asio::ip::tcp::socket;
            static auto ctx(U* sock) -> decltype(auto)
            {
                return sock->get_executor();
            }
        };
    }

    /**
     *  A http_connection_base provides common functionality for tcp connections.
     */
    template <typename SocketT>
    class http_connection_base : public http_connection_interface
    {
        friend http_basic_server;
        friend http_server;
        friend http_secure_server;
        friend http_stream_device;

    public:
        using socket_type = SocketT;

    public:

        explicit http_connection_base(http_server_interface* parent, SocketT* socket, final_callback const& on_timeout)
            : parent_(parent)
            , socket_{socket}
            , buffer_(config::buffer_size)
            , write_buffer_{}
            , read_callback_inst_{}
            , bytes_ready_{0}
            , read_timeout_timer_{internal::get_executor <SocketT>::ctx(socket)}
            , closed_{false}
            , kept_alive_{nullptr}
            , on_timeout_{on_timeout}
        {
            // this will ensure, that the timer does not fire right away when it it started
            // the timer is started on read, but the timeout will be set later in every do_read cycle.
            read_timeout_timer_.expires_at(boost::posix_time::pos_infin);
        }

        explicit http_connection_base(
            http_server_interface* parent,
            SocketT&& socket,
            SocketT&& /*dummy hack*/,
            final_callback const& on_timeout
        )
            : http_connection_base(parent, new SocketT(std::move(socket)), on_timeout)
        {
            // DELEGATE
        }


        ~http_connection_base()
        {
            stop();

            // This must be the last action of this function
            kept_alive_.reset(nullptr);
        }

        /**
         *  Starts asynchronous reading.
         */
        void start() override
        {
            // do_read();
        }

        /**
         *  Closes the socket and aborts all messaging
         *  Also removes the livetime bindings
         */
        void stop() override
        {
            if (closed_.load())
                return;
            closed_.store(true);

            shutdown();
            internal::get_socket_layer(*socket_).close();
            read_timeout_timer_.cancel();
        }

        /**
         *  Gracefully shuts the socket down.
         */
        // void shutdown();

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

            boost::asio::async_write(*socket_, boost::asio::buffer(write_buffer_),
                [handler](boost::system::error_code ec, std::size_t amount)
                {
                    handler(ec, amount);
                }
            );
        }

        /**
         *  A write function for pure data.
         *  This function assumes that passed data survives the entire ordeal.
         */
        void write(std::vector <char>&& eol_container, write_callback handler) override
        {
            write_buffer_ = std::move(eol_container);

            boost::asio::async_write
            (
                *socket_,
                boost::asio::buffer(write_buffer_),
                [handler](boost::system::error_code ec, std::size_t amount)
                {
                    handler(ec, amount);
                }
            );
        }

        /**
         *  This function writes the whole container onto the stream.
         *  The handler function is called when the write operation completes.
         *  Do not (!) call write while another write operation is in progress!
         */
        void write(std::vector <char> const& container, write_callback handler) override
        {
            write(std::begin(container), std::end(container), handler);
        }

        /**
         *  This function writes the istream onto the tcp stream.
         *  The handler function is called when the write operation completes.
         *  Do not (!) call write while another write operation is in progress!
         */
        void write(std::istream& stream, write_callback handler) override
        {
            write_buffer_.resize(config::buffer_size);
            stream.read(write_buffer_.data(), config::buffer_size);

            if (stream.gcount() != 0)
            {
                boost::asio::async_write(*socket_, boost::asio::buffer(write_buffer_),
                    [this, cb{handler}, &stream](boost::system::error_code ec, std::size_t amount)
                    {
                        if (!ec)
                        {
                            if (stream.gcount() == config::buffer_size)
                                write(stream, cb);
                            else
                                cb(ec, amount);
                        }
                        else
                        {
                            // If you crash in here your connection is already dead.
                            // The error message is something like "write on shutdown connection" or
                            // "ssl_writer_interal:protocol is shutdown". Do NOT! use send/send_file/end multiple times.
                            cb(ec, amount);
                        }
                    }
                );
            }
            else
            {
                handler({}, 0);
            }
        }

        /**
         *  This function writes the string onto the tcp stream.
         *  The handler function is called when the write operation completes.
         *  Do not (!) call write while another write operation is in progress!
         */
        void write(std::string const& string, write_callback handler) override
        {
            write(std::begin(string), std::end(string), handler);
        }

        /**
         *  Sets the read callback, which is called when a read operation finishes.
         */
        void set_read_callback(read_callback const& new_read_callback) override
        {
            read_callback_inst_ = new_read_callback;
        }

        /**
         *  Read more bytes into the buffer. This will overwrite the buffer.
         */
        void read() override
        {
            read_timeout_timer_.async_wait(
                [this](boost::system::error_code const& ec)
                {
                    check_deadline(&read_timeout_timer_, ec);
                }
            );


            do_read();
        }

        /**
         *  Returns the amount of bytes that remain in the read buffer.
         *
         *  @return A number of bytes that can be taken from the buffer.
         */
        std::size_t ready_count() const override
        {
            return bytes_ready_;
        }

        /**
         *  Returns the beginning of the read buffer.
         *
         *  @return An iterator to the read buffer.
         */
        buffer_iterator begin() const override
        {
            return std::cbegin(buffer_);
        }

        /**
         *  Returns the end of the read buffer.
         *
         *  @return An iterator to the read buffer.
         */
        buffer_iterator end() const override
        {
            return std::cbegin(buffer_) + bytes_ready_;
        }

        /**
         *  Returns the read buffer.
         *  Used in request_handler class.
         */
        std::vector <char>& get_read_buffer() override
        {
            return buffer_;
        }

        /**
         *  Returns the tcp server behind this tcp connection.
         *  Do not abuse.
         */
        http_server_interface* get_parent() override
        {
            return parent_;
        }

        /**
         *  Return the associated socket.
         */
        boost::asio::ip::tcp::socket::lowest_layer_type* get_socket() override
        {
            return &internal::get_socket_layer(*socket_);
        }

        /**
         *  Returns the remote host address.
         *  @return empty string on fail, or address on success.
         */
        std::string get_remote_address() const override
        {
            boost::system::error_code ec;
            auto endpoint = internal::get_socket_layer(*socket_).remote_endpoint(ec);

            if (ec)
                return "";

            return endpoint.address().to_string();
        }

        /**
         *  Returns the remote host port.
         *  @return 0 on failure, or any other number that is the port (on success).
         */
        unsigned short get_remote_port() const override
        {
            boost::system::error_code ec;
            auto endpoint = internal::get_socket_layer(*socket_).remote_endpoint(ec);

            if (ec)
                return 0;

            return endpoint.port();
        }

    private:
        void do_read()
        {
            // sets / resets the timer.
            read_timeout_timer_.expires_from_now(boost::posix_time::seconds(config::read_timeout));

            socket_->async_read_some(boost::asio::buffer(buffer_),
                [this](boost::system::error_code ec, std::size_t bytes_transferred)
                {
                    bytes_ready_ = bytes_transferred;
                    read_timeout_timer_.expires_from_now(boost::posix_time::pos_infin);
                    read_callback_inst_(ec, bytes_transferred);
                }
            );
        }

        void attach_lifetime_binder(lifetime_binding* ltb)
        {
            kept_alive_ = std::unique_ptr <lifetime_binding> (ltb);
        }

        /**
        *  Checks the deadline_timer and possibly terminates the connection if the timeout was reached.
        *  Any IO will reset the deadline
        */
        void check_deadline(boost::asio::deadline_timer* timer, boost::system::error_code const& ec)
        {
            // The operation was aborted. This most likely means, that the connection was terminated.
            // Accessing this from here is unsafe.
            if (ec == boost::asio::error::operation_aborted)
                return;

            // The socket is not open anymore, therefore timeout checkings are no longer relevant.
            if (stopped())
                return;

            //if (ec == boost::system::errc::operation_canceled)
            //    return;

            if (ec && ec != boost::system::errc::operation_canceled)
            {
                // There has been an error, no matter what, close the connection.
                DUMP(ec, ATTENDER_CODE_PLACE);

                stop();
                return;
            }

            // Check whether the deadline has passed. We compare the deadline against
            // the current time since a new asynchronous operation may have moved the
            // deadline before this actor had a chance to run.
            if (timer->expires_at() <= boost::asio::deadline_timer::traits_type::now())
            {
                if (on_timeout_)
                    on_timeout_(&get_request_handler(), &get_response_handler());
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

        response_handler& get_response_handler() override
        {
            return kept_alive_->get_response_handler();
        }
        request_handler& get_request_handler() override
        {
            return kept_alive_->get_request_handler();
        }

        /**
        *  Returns wether the socket is open or not.
        */
        bool stopped() const override
        {
            return !internal::get_socket_layer(*socket_).is_open();
        }

    protected:
        http_server_interface* parent_;
        std::unique_ptr <SocketT> socket_;
        std::vector <char> buffer_;
        std::vector <char> write_buffer_;
        read_callback read_callback_inst_;
        std::size_t bytes_ready_;
        boost::asio::deadline_timer read_timeout_timer_;
        std::atomic_bool closed_;
        std::unique_ptr <lifetime_binding> kept_alive_;
        final_callback on_timeout_;
    };

    class http_stream_device
    {
    public:
        using char_type = char;
        using category = boost::iostreams::source_tag;

        http_stream_device(http_connection_interface* connection)
            : connection_{connection}
            , pos_{0}
        {
        }

        std::streamsize read(char_type* s, std::streamsize n)
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

    private:
        http_connection_interface* connection_;
        std::size_t pos_;
    };
}
