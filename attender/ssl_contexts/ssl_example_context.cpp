#include "ssl_example_context.hpp"

#include <stdexcept>

namespace ssl = boost::asio::ssl;

namespace attender
{
//#####################################################################################################################
    ssl_example_context::ssl_example_context(std::string const& private_key_file, std::string const& certificate_file, std::string passphrase)
        : ssl_example_context(
            std::move(private_key_file),
            std::move(certificate_file),
            [pass=std::move(passphrase)](std::size_t maxLength, ...) -> std::string {
                if (pass.length() > maxLength)
                    throw std::invalid_argument("passphrase is too long");
                return pass;
            }
        )
    {
        // DELEGATION! DO NOT PUT ANYTHING HERE
    }
//---------------------------------------------------------------------------------------------------------------------
    ssl_example_context::ssl_example_context(std::string const& private_key_file, std::string const& certificate_file)
        : context_{boost::asio::ssl::context::sslv23}
    {
        common_context_initialization(private_key_file, certificate_file);
    }
//---------------------------------------------------------------------------------------------------------------------
    ssl_example_context::ssl_example_context(std::string const& private_key_file, std::string const& certificate_file, password_callback_type passphrase_provider)
        : context_{boost::asio::ssl::context::sslv23}
    {
        context_.set_password_callback(std::move(passphrase_provider));

        common_context_initialization(private_key_file, certificate_file);
    }
//---------------------------------------------------------------------------------------------------------------------
    void ssl_example_context::common_context_initialization(std::string const& private_key_file, std::string const& certificate_file)
    {
        context_.set_options(
                ssl::context::default_workarounds
            //|   ssl::context::no_sslv2
            |   ssl::context::single_dh_use
            |   ssl::context::no_compression
        );

        context_.set_verify_mode(ssl::verify_none); // no client certificates

        context_.use_certificate_chain_file(certificate_file.c_str());
        context_.use_private_key_file(private_key_file.c_str(), ssl::context::pem);
    }
//---------------------------------------------------------------------------------------------------------------------
    boost::asio::ssl::context* ssl_example_context::get_ssl_context()
    {
        return &context_;
    }
//#####################################################################################################################
}
