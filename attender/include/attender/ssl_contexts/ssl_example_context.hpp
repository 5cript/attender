#pragma once

#include <attender/ssl_contexts/ssl_context_interface.hpp>

#include <functional>
#include <string>

namespace attender
{
    /**
     *  This is an example implementation for an ssl context, that uses server certificates,
     *  but no client certificates. A passphrase can be provided.
     *  You should probably use your own implementation for the ssl_context_interface,
     *  as I cannot guarantee having no security flaws in this implementation.
     *  Therefore I chose to call this "ssl_example_context", even though it is a typical implementation
     *  for regular HTTPS applications.
     *
     *  It only supports PEM format keys, not ASN1, even though boost does.
     */
    class ssl_example_context : public ssl_context_interface
    {
    public:
        using password_callback_type = std::function <std::string(std::size_t /* maxLength */, boost::asio::ssl::context::password_purpose /* purpose */)>;

    public:
        ssl_example_context(std::string const& private_key_file, std::string const& certificate_file, std::string passphrase);
        ssl_example_context(std::string const& private_key_file, std::string const& certificate_file, password_callback_type passphrase_provider);
        ssl_example_context(std::string const& private_key_file, std::string const& certificate_file);
        ~ssl_example_context() = default;

        boost::asio::ssl::context* get_ssl_context() override;

    private:
        void common_context_initialization(std::string const& private_key_file, std::string const& certificate_file);

    private:
        boost::asio::ssl::context context_;
    };
}
