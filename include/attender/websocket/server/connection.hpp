#pragma once

#include <attender/net_core.hpp>
#include <attender/websocket/server/session_base.hpp>
#include <attender/websocket/server/noop_session.hpp>

#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <attender/websocket/server/security.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>

#include <memory>
#include <functional>
#include <atomic>
#include <variant>
#include <optional>

namespace attender::websocket
{
    namespace detail
    {
        // helper type for the visitor #4
        template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
        // explicit deduction guide (not needed as of C++20)
        template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;
    }

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
            session_ = std::make_shared<T>(this, std::forward<Args>(args)...);
            return *static_cast<T*>(session_.get());
        }

        connection(
            boost::asio::io_context* ctx,
            boost::asio::ip::tcp::socket&& socket,
            std::function <void(boost::system::error_code ec)> on_accept_error,
            std::function <void(std::shared_ptr<connection>)> on_accept,
            boost::asio::ssl::context& securityContext
        )
            : ws_{
                boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>>{std::move(socket), securityContext}
            }
            , strand_(boost::asio::make_strand(*ctx))
            , read_buffer_{}
            , write_buffer_{}
            , on_accept_error_{std::move(on_accept_error)}
            , on_accept_{std::move(on_accept)}
            , session_{std::make_shared<noop_session>(this)}
        {
        }
        
        connection(
            boost::asio::io_context* ctx,
            boost::asio::ip::tcp::socket&& socket,
            std::function <void(boost::system::error_code ec)> on_accept_error,
            std::function <void(std::shared_ptr<connection>)> on_accept
        )
            : ws_{
                boost::beast::websocket::stream<boost::beast::tcp_stream>{std::move(socket)}
            }
            , strand_(boost::asio::make_strand(*ctx))
            , read_buffer_{}
            , write_buffer_{}
            , on_accept_error_{std::move(on_accept_error)}
            , on_accept_{std::move(on_accept)}
            , session_{std::make_shared<noop_session>(this)}
        {
        }

        void start()
        {
            with_stream_do([this](auto& ws) {
                boost::asio::dispatch(ws.get_executor(), [self = shared_from_this()](){
                    self->with_stream_do([&self](auto& ws) {
                        if (std::holds_alternative<boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>>>(self->ws_))
                            self->ssl_handshake();
                        else
                            self->accept();
                    });
                });
            });
        }
        
        void close()
        {
            with_stream_do([this](auto& ws) {
                ws.async_close(
                    boost::beast::websocket::close_code::normal,
                    [shared = shared_from_this()](boost::beast::error_code){
                        shared->session_->on_close();
                    }
                );
            });
        }

        template <typename FuncT>
        void with_stream_do(FuncT&& func) {
            std::visit(std::forward<FuncT>(func), ws_);
        }

    private:
        void ssl_handshake()
        {
            // cannot apply follwing operation on both types of socket.
            std::visit(detail::overloaded{
                [this](boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>>& ws)
                {
                    boost::beast::get_lowest_layer(ws).expires_after(std::chrono::seconds(30));

                    ws.next_layer().async_handshake(
                        boost::asio::ssl::stream_base::server,
                        boost::beast::bind_front_handler(
                            &connection::on_ssl_handshake,
                            shared_from_this()
                        )
                    );
                },
                [](auto&)
                {
                    // should never end up here.
                }
            }, ws_);
        }

        void on_ssl_handshake(boost::beast::error_code ec)
        {
            if(ec)
                return on_accept_error_(ec);

            with_stream_do([this](auto& ws) {
                boost::beast::get_lowest_layer(ws).expires_never();

                ws.set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::server));

                ws.set_option(boost::beast::websocket::stream_base::decorator(
                    [](boost::beast::websocket::response_type& res)
                    {
                        res.set(boost::beast::http::field::server,
                            std::string(BOOST_BEAST_VERSION_STRING) + " websocket-server-async-ssl");
                    })
                );
            });

            accept();
        }

        void accept()
        {
            with_stream_do([this](auto& ws) {
                ws.async_accept(
                    boost::asio::bind_executor(
                        strand_,
                        std::bind(
                            &connection::on_accept,
                            shared_from_this(),
                            std::placeholders::_1
                        )
                    )
                );
            });
        }

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
            with_stream_do([this](auto& ws) {
                ws.async_read(
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
            });
        }

        void
        on_read(
            boost::system::error_code ec,
            std::size_t bytes_received)
        {
            // This indicates that the session was closed
            //if(ec == boost::beast::websocket::error::closed)
            //    return session_->on_close();

            if(ec)
                return session_->on_close();

            with_stream_do([this, &bytes_received](auto& ws) {
                if (ws.got_text())
                {
                    session_->on_text({static_cast <const char*>(read_buffer_.data().data()), bytes_received});
                }
                else 
                {
                    session_->on_binary(static_cast <const char*>(read_buffer_.data().data()), bytes_received);
                }
            });

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
        std::variant<
            boost::beast::websocket::stream<boost::beast::tcp_stream>,
            boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>>
        > ws_;
        boost::asio::strand<boost::asio::io_context::executor_type> strand_;
        boost::beast::flat_buffer read_buffer_;
        boost::beast::flat_buffer write_buffer_;
        std::function <void(boost::system::error_code ec)> on_accept_error_;
        std::function <void(std::shared_ptr<connection>)> on_accept_;
        std::shared_ptr <session_base> session_;
        std::atomic_bool write_in_progress_;
        std::optional<security_parameters> security_parameters_;
    };

}