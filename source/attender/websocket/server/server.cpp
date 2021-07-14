#include <attender/websocket/server/server.hpp>

namespace attender::websocket
{
//#####################################################################################################################
    struct server::implementation : public std::enable_shared_from_this<server::implementation>
    {
        boost::asio::io_context* service;
        boost::asio::ip::tcp::acceptor acceptor;
        boost::asio::ip::tcp::endpoint local_endpoint;
        std::function <void(boost::system::error_code)> on_error;
        std::function<void(std::shared_ptr<connection>)> on_connection;

        implementation(
            boost::asio::io_context* service,
            std::function <void(boost::system::error_code)> on_error
        )
            : service{service}
            , acceptor{*service}
            , local_endpoint{}
            , on_error{std::move(on_error)}
            , on_connection{}
        {
        }

        void do_accept()
        {
            acceptor.async_accept(
                boost::asio::make_strand(*service),
                [weak = weak_from_this()](boost::system::error_code ec, boost::asio::ip::tcp::socket&& socket)
                {
                    if (auto shared = weak.lock(); shared)
                    {
                        if (ec)
                        {
                            shared->on_error(ec);
                        }
                        std::make_shared<connection>(shared->service, std::move(socket), shared->on_error, shared->on_connection)->start();
                        shared->do_accept();
                    }
                }
            );
        }
    };
//#####################################################################################################################
    server::server(boost::asio::io_context* service, std::function <void(boost::system::error_code)> on_error)
        : impl_{std::make_shared <implementation>(service, std::move(on_error))}
    {        
    }
//---------------------------------------------------------------------------------------------------------------------
    server::~server()
    {
        stop();
    }
//---------------------------------------------------------------------------------------------------------------------
    boost::asio::ip::tcp::endpoint server::local_endpoint() const
    {
        return impl_->acceptor.local_endpoint();
    }
//---------------------------------------------------------------------------------------------------------------------
    void server::start(
        std::function<void(std::shared_ptr<connection>)> on_connection,
        std::string const& port, 
        std::string const& host
    )
    {
        stop();

        boost::asio::ip::tcp::resolver resolver{*impl_->service};

        impl_->on_connection = on_connection;

        impl_->local_endpoint = *resolver.resolve(host, port);
        impl_->acceptor.open(impl_->local_endpoint.protocol());
        impl_->acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        impl_->acceptor.bind(impl_->local_endpoint);
        impl_->acceptor.listen();

        impl_->do_accept();
    }
//---------------------------------------------------------------------------------------------------------------------
    void server::stop()
    {
        impl_->acceptor.close();
    }
//#####################################################################################################################
}