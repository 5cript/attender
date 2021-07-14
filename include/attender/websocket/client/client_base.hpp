#pragma once

#include <attender/net_core.hpp>
#include <attender/utility/visit_overloaded.hpp>
#include <attender/ssl_contexts/ssl_context_interface.hpp>

#include <boost/beast/core.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/core/stream_traits.hpp>

#include <chrono>
#include <memory>
#include <variant>
#include <type_traits>

namespace attender::websocket
{
    template <typename SocketT>
    class client_base
    {
    public:
        constexpr static bool is_secure = std::is_same_v<SocketT, boost::beast::websocket::stream<boost::asio::ip::tcp::socket>>;

        struct connection_parameters
        {
            std::string host = "::";
            std::string port = "";
            std::string target = "/";
            std::chrono::seconds secureAsyncOpsExpiration = std::chrono::seconds{3};
        };

        struct timeouts
        {
            std::optional <std::chrono::milliseconds> handshake_timeout{std::nullopt};
            std::optional <std::chrono::milliseconds> idle_timeout{std::nullopt};
        };
        
    protected:
        struct implementation
            : public std::enable_shared_from_this<client_base::implementation>
        {
            asio::io_service* service_;
            std::unique_ptr <ssl_context_interface> ssl_context_;
            SocketT ws_;
            boost::asio::ip::tcp::endpoint endpoint_;
            boost::beast::flat_buffer read_buffer_;
            std::function<void(boost::system::error_code, std::string const&)> read_cb_;

            void set_user_agent()
            {
                ws_.set_option(boost::beast::websocket::stream_base::decorator{
                    [](boost::beast::websocket::request_type& req)
                    {
                        req.set(boost::beast::http::field::user_agent, std::string{BOOST_BEAST_VERSION_STRING} + " attender_wsc");
                    }
                });
            }

            void on_read(boost::beast::error_code ec, std::size_t bytes_transferred)
            {
                if (ec)
                    return read_cb_(ec, "");

                read_cb_(ec, boost::beast::buffers_to_string(read_buffer_.data()));
                read_buffer_.consume(bytes_transferred);

                ws_.async_read(read_buffer_, [impl_{this->shared_from_this()}](auto&&... args){
                    impl_->on_read(std::forward<decltype(args)>(args)...);            
                });
            }
            
            boost::system::error_code setTlsHostname(char const* host)
            {
                if (!SSL_set_tlsext_host_name(ws_.next_layer().native_handle(), host))
                {
                    return boost::system::error_code{
                        static_cast <int>(::ERR_get_error()),
                        boost::asio::error::get_ssl_category()
                    };
                }
                return boost::system::error_code{0, boost::asio::error::get_ssl_category()};
            }

            implementation(asio::io_service* service)
                : service_{service}
                , ssl_context_{}
                , ws_{*service}
                , endpoint_{}
                , read_buffer_{}
                , read_cb_{}
            {}

            implementation(asio::io_service* service, std::unique_ptr <ssl_context_interface>&& ssl_context)
                : service_{service}
                , ssl_context_{std::move(ssl_context)}
                , ws_{*service, *ssl_context_->get_ssl_context()}
                , endpoint_{}
                , read_buffer_{}
                , read_cb_{}
            {}
        };

        std::shared_ptr <implementation> impl_;

    protected:
        client_base(asio::io_service* service, std::unique_ptr <ssl_context_interface>&& ssl_context)
            : impl_{std::make_shared<client_base::implementation>(service, std::move(ssl_context))}
        {
        }

    public:
        /**
         * Construct a client with an io_service and an error handler for errors occuring from
         * sources other than functions this class provides. For instance the destructor can cause errors.
         * 
         * @param service A boost io_service
         * @param errorHandler A function taking a cstring and an error code and a message
         */
        explicit client_base(asio::io_service* service)
            : impl_{std::make_shared<client_base::implementation>(service)}
        {
        }
        virtual ~client_base()
        {
            // there should never be an exception, but lets not std::terminate by accident.
            try
            {
                disconnect();
            }
            catch(...)
            {
            }
        }

        client_base(client_base const&) = delete;
        client_base(client_base&&) = default;
        client_base& operator=(client_base const&) = delete;
        client_base& operator=(client_base&&) = default;

        /**
         * Connect to a remote websocket server.
         */
        virtual boost::system::error_code connect_sync(connection_parameters const& params) = 0;

        /**
         * Set timeouts for handshakes and idling.
         */
        void set_timeout(timeouts const& timeouts)
        {
            boost::beast::websocket::stream_base::timeout opt{
                timeouts.handshake_timeout ? boost::beast::websocket::stream_base::none() : *timeouts.handshake_timeout,
                timeouts.idle_timeout ? boost::beast::websocket::stream_base::none() : *timeouts.idle_timeout,
                false
            };

            impl_->ws_.set_option(opt);
        }

        /**
         * Disconnect from the server.
         */
        boost::system::error_code disconnect()
        {
            boost::system::error_code ec;
            if (impl_->ws_.is_open())
                impl_->ws_.close(boost::beast::websocket::close_code::normal, ec);
            return ec;
        }

        /**
         * Returns the underlying boost websocket instance.
         */
        boost::beast::websocket::stream<boost::asio::ip::tcp::socket>& lower_layer()
        {
            return impl_->ws_;
        }

        void set_binary_mode(bool binary)
        {
            impl_->ws_.binary(binary);
        }

        /**
         * Writes string to the server.
         */
        void write_sync(std::string const& data)
        {
            impl_->ws_.write(boost::asio::buffer(data));
        }

        /**
         * Writes string to the server.
         * @param data Data to write.
         * @param on_completion What to do when complete.
         */
        void write(std::string const& data, std::function <void(const boost::system::error_code&, std::size_t)> const& on_completion)
        {
            std::shared_ptr <const std::string> buf = std::make_shared <const std::string>(data);
            impl_->ws_.
            impl_->ws_.async_write(boost::asio::buffer(buf->data(), buf->size()), [buf, on_completion]
            (
                boost::system::error_code const& ec,
                std::size_t amountWritten
            )
            {
                on_completion(ec, amountWritten);
            });
        }

        /**
         * Starts reading.
         */
        void listen(std::function<void(boost::system::error_code, std::string const&)> read_cb)
        {            
            impl_->read_cb_ = std::move(read_cb);
            impl_->ws_.async_read(impl_->read_buffer_, [impl_{this->impl_}](auto&&... args){
                impl_->on_read(std::forward<decltype(args)>(args)...);
            });
        }

    protected:
        std::variant<boost::asio::ip::tcp::resolver::results_type, boost::system::error_code> resolve(connection_parameters const& params) const
        {
            boost::system::error_code ec;
            boost::asio::ip::tcp::resolver resolver{*impl_->service_};
            auto results = resolver.resolve(params.host, params.port, ec);
            if (ec)
                return ec;
            return results;
        }
    };
}
