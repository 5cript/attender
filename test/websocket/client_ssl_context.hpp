#pragma once

#include <attender/ssl_contexts/ssl_context_interface.hpp>

class ssl_client_context : public attender::ssl_context_interface
{
public:
    ssl_client_context()
        : context_{boost::asio::ssl::context::tlsv13_client}
    {
        context_.set_options(
                boost::asio::ssl::context::default_workarounds
            |   boost::asio::ssl::context::single_dh_use
            |   boost::asio::ssl::context::no_compression
        );

        context_.set_password_callback([](std::size_t /* maxLength */, boost::asio::ssl::context::password_purpose /* purpose */){
            return "asdf";
        });
        context_.set_verify_mode(boost::asio::ssl::verify_none);
    }

    boost::asio::ssl::context* get_ssl_context() override
    {
        return &context_;
    }

private:
    boost::asio::ssl::context context_;
};