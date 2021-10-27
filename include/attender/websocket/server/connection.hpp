#pragma once

#include <attender/net_core.hpp>
#include <attender/websocket/server/session_base.hpp>
#include <attender/websocket/server/noop_session.hpp>

#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/strand.hpp>

#include <memory>
#include <functional>
#include <atomic>

namespace attender::websocket
{
    class connection : public std::enable_shared_from_this<connection>
    {
    public:
        friend class session_base;
        friend class server;

        /**
         *  Fill this connection with self defined session that receives events and write data.
         */
        template <typename T, typename... Args>
        T& create_session(Args&&... args)
        {
            session_ = std::make_unique<T>(this, std::forward<Args>(args)...);
            return *static_cast<T*>(session_.get());
        }
        
        connection(
            boost::asio::io_context* ctx,
            boost::asio::ip::tcp::socket&& socket,
            std::function <void(boost::system::error_code ec)> on_accept_error,
            std::function <void(std::shared_ptr<connection>)> on_accept
        )
            : socket_(std::move(socket))
            , ws_(socket_)
            , strand_(boost::asio::make_strand(*ctx))
            , read_buffer_{}
            , write_buffer_{}
            , on_accept_error_{std::move(on_accept_error)}
            , on_accept_{std::move(on_accept)}
            , session_{std::make_unique<noop_session>(this)}
        {
        }

        void start()
        {
            ws_.async_accept(
                boost::asio::bind_executor(
                    strand_,
                    std::bind(
                        &connection::on_accept,
                        shared_from_this(),
                        std::placeholders::_1
                    )
                )
            );
        }

        void close()
        {
            ws_.async_close(
                boost::beast::websocket::close_code::normal,
                [shared = shared_from_this()](boost::beast::error_code){
                    shared->session_->on_close();
                }
            );
        }

    private:
        void
        on_accept(boost::system::error_code ec)
        {
            if(ec)
                return on_accept_error_(ec);

            on_accept_(shared_from_this());
            do_read();
        }

        void
        do_read()
        {
            ws_.async_read(
                read_buffer_,
                boost::asio::bind_executor(
                    strand_,
                    std::bind(
                        &connection::on_read,
                        shared_from_this(),
                        std::placeholders::_1,
                        std::placeholders::_2
                    )
                )
            );
        }

        void
        on_read(
            boost::system::error_code ec,
            std::size_t bytes_received)
        {
            // This indicates that the session was closed
            if(ec == boost::beast::websocket::error::closed)
                return session_->on_close();

            if(ec)
                session_->on_error(ec, "read");

            if (ws_.got_text())
            {
                session_->on_text({static_cast <const char*>(read_buffer_.data().data()), bytes_received});
            }
            else 
            {
                session_->on_binary(static_cast <const char*>(read_buffer_.data().data()), bytes_received);
            }

            read_buffer_.consume(read_buffer_.size());
            do_read();
        }

        void
        on_write(
            boost::system::error_code ec,
            std::size_t bytes_transferred)
        {
            write_in_progress_.store(false);
            write_buffer_.consume(write_buffer_.size());

            if(ec)
                return session_->on_error(ec, "write");

            session_->on_write_complete(bytes_transferred);
        }
        
    private:
        boost::asio::ip::tcp::socket socket_;
        boost::beast::websocket::stream<boost::asio::ip::tcp::socket&> ws_;
        boost::asio::strand<boost::asio::io_context::executor_type> strand_;
        boost::beast::flat_buffer read_buffer_;
        boost::beast::flat_buffer write_buffer_;
        std::function <void(boost::system::error_code ec)> on_accept_error_;
        std::function <void(std::shared_ptr<connection>)> on_accept_;
        std::unique_ptr <session_base> session_;
        std::atomic_bool write_in_progress_;
    };

}