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

    class connection;

    /// Not yet a websocket.
    class proto_connection : public  std::enable_shared_from_this<proto_connection>
    {
    public:
        proto_connection(
            boost::asio::io_context* ctx,
            boost::asio::ip::tcp::socket&& socket,
            std::function <void(boost::system::error_code ec)> on_accept_error,
            std::function <void(std::shared_ptr<connection>)> on_accept,
            std::unique_ptr<boost::asio::ssl::context>&& security_context
        )
            : ctx_{ctx}
            , security_context_{std::move(security_context)}
            , socket_{[&socket, this]() -> decltype(socket_) {
                if (security_context_)
                    return boost::beast::ssl_stream<boost::beast::tcp_stream>{std::move(socket), *security_context_};
                else
                    return boost::beast::tcp_stream{std::move(socket)};
            }()}
            , on_accept_error_{std::move(on_accept_error)}
            , on_accept_{std::move(on_accept)}
            , upgrade_request_{}
        {}

        void start()
        {
            boost::asio::dispatch(*ctx_, [self = shared_from_this()]()
            {
                if (self->security_context_)
                    self->ssl_handshake();
                else
                    self->read_upgrade();
            });
        }

    private:
        void ssl_handshake()
        {
            // cannot apply follwing operation on both types of socket.
            std::visit(detail::overloaded{
                [this](boost::beast::ssl_stream<boost::beast::tcp_stream>& sock)
                {
                    boost::beast::get_lowest_layer(sock).expires_after(std::chrono::seconds(30));

                    sock.async_handshake(
                        boost::asio::ssl::stream_base::server,
                        [self = shared_from_this()]<typename T>(T&& ec)
                        {
                            self->on_ssl_handshake(std::forward<T>(ec));
                        }
                    );
                },
                [](auto&)
                {
                    // should never end up here.
                }
            }, socket_);
        }

        void on_ssl_handshake(boost::beast::error_code ec)
        {
            if(ec)
                return on_accept_error_(ec);

            with_stream_do([this](auto& sock) {
                boost::beast::get_lowest_layer(sock).expires_never();
            });

            read_upgrade();
        }

        void read_upgrade()
        {
            with_stream_do([this](auto& sock) {
                auto buffer = std::make_shared<boost::beast::flat_buffer>();
                boost::beast::http::async_read(
                    sock, 
                    *buffer, 
                    upgrade_request_,
                    [buffer, self = shared_from_this()](boost::system::error_code ec, std::size_t bytes_transferred)
                    {
                        if (ec)
                            return self->on_accept_error_(ec);
                            
                        if (boost::beast::websocket::is_upgrade(self->upgrade_request_))
                            self->on_upgrade();
                        else
                            self->close();
                    }
                );
            });
        }

        void on_upgrade();
        
        void close()
        {
            std::visit(detail::overloaded{
                [](boost::beast::tcp_stream& stream) {stream.close();},
                [](boost::beast::ssl_stream<boost::beast::tcp_stream>& stream) {stream.next_layer().close();},
            }, socket_);
        }

        template <typename FuncT>
        void with_stream_do(FuncT&& func) {
            std::visit(std::forward<FuncT>(func), socket_);
        }

    private:
        boost::asio::io_context* ctx_;
        std::unique_ptr<boost::asio::ssl::context> security_context_;
        std::variant<
            boost::beast::tcp_stream,
            boost::beast::ssl_stream<boost::beast::tcp_stream>
        > socket_;
        std::function <void(boost::system::error_code ec)> on_accept_error_;
        std::function <void(std::shared_ptr<connection>)> on_accept_;
        boost::beast::http::request<boost::beast::http::string_body> upgrade_request_;
    };

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
            boost::beast::ssl_stream<boost::beast::tcp_stream>&& socket,
            std::function <void(boost::system::error_code ec)> on_accept_error,
            std::function <void(std::shared_ptr<connection>)> on_accept,
            boost::beast::http::request<boost::beast::http::string_body> upgrade_request
        )
            : ws_{
                boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>>{std::move(socket)}
            }
            , strand_(boost::asio::make_strand(*ctx))
            , read_buffer_{}
            , write_buffer_{}
            , on_accept_error_{std::move(on_accept_error)}
            , on_accept_{std::move(on_accept)}
            , session_{std::make_shared<noop_session>(this)}
            , upgrade_request_{std::move(upgrade_request)}
            , was_closed_{false}
        {
        }
        
        connection(
            boost::asio::io_context* ctx,
            boost::beast::tcp_stream&& socket,
            std::function <void(boost::system::error_code ec)> on_accept_error,
            std::function <void(std::shared_ptr<connection>)> on_accept,
            boost::beast::http::request<boost::beast::http::string_body> upgrade_request
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
            , upgrade_request_{std::move(upgrade_request)}
            , was_closed_{false}
        {
        }

        void start()
        {
            with_stream_do([this](auto& ws) {
                ws.set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::server));

                ws.set_option(boost::beast::websocket::stream_base::decorator(
                    [](boost::beast::websocket::response_type& res)
                    {
                        res.set(boost::beast::http::field::server,
                            std::string(BOOST_BEAST_VERSION_STRING) + " websocket-server-async-ssl");
                    })
                );

                ws.async_accept(
                    upgrade_request_,
                    [self = shared_from_this()]<typename T>(T&& ec)
                    {
                        self->on_accept(std::forward<T>(ec));
                    }
                ); 
            });
        }

        ~connection()
        {
            close_sync();
        }
        
        void close()
        {
            if (was_closed_)            
                return;

            with_stream_do([this](auto& ws) {
                ws.async_close(
                    boost::beast::websocket::close_code::normal,
                    [shared = shared_from_this()](boost::beast::error_code){
                        shared->was_closed_ = true;
                        shared->session_->on_close();
                    }
                );
            });
        }

        void close_sync()
        {
            if (was_closed_)            
                return;

            with_stream_do([this](auto& ws) {
                boost::beast::error_code ec;
                ws.close(boost::beast::websocket::close_code::normal, ec);
                was_closed_ = true;
                session_->on_close();
            });
        }

        template <typename FuncT>
        void with_stream_do(FuncT&& func) {
            std::visit(std::forward<FuncT>(func), ws_);
        }

        boost::beast::http::request<boost::beast::http::string_body> const& get_upgrade_request() const
        {
            return upgrade_request_;
        }

    private:
        void on_accept(boost::system::error_code ec)
        {
            if(ec)
                return on_accept_error_(ec);

            on_accept_(shared_from_this());
            do_read();
        }

        void do_read()
        {
            with_stream_do([this](auto& ws) {
                ws.async_read(
                    read_buffer_,
                    boost::asio::bind_executor(
                        strand_,
                        [self = shared_from_this()]<typename T, typename U>(T&& t, U&& u)
                        {
                            self->on_read(std::forward<T>(t), std::forward<U>(u));
                        }
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
        boost::beast::http::request<boost::beast::http::string_body> upgrade_request_;
        bool was_closed_;
    };
}