#include "response.hpp"
#include "tcp_connection.hpp"
#include "tcp_server.hpp"
#include "mime.hpp"

#include <iostream>

namespace attender
{
//#####################################################################################################################
    /**
     *  Trick to hide template in c++ file. So that tcp_connection.hpp is not required in the header.
     */
    template <typename T>
    void write(response_handler* repHandler, std::shared_ptr <T> data)
    {
        repHandler->send_header([repHandler, data](boost::system::error_code ec){
            // end the connection, on error
            if (ec)
            {
                repHandler->end();
                return;
            }

            repHandler->get_connection()->write(*data, [repHandler](boost::system::error_code ec){
                // end no matter what.
                repHandler->end();
            });
        });
    }
//#####################################################################################################################
    response_handler::response_handler(tcp_connection_interface* connection) noexcept
        : connection_{connection}
        , header_{}
        , headerSent_{false}
    {

    }
//---------------------------------------------------------------------------------------------------------------------
    response_handler::~response_handler()
    {
        std::cout << "response destroyed\n";
    }
//---------------------------------------------------------------------------------------------------------------------
    tcp_connection_interface* response_handler::get_connection()
    {
        return connection_;
    }
//---------------------------------------------------------------------------------------------------------------------
    response_handler&  response_handler::append(std::string const& field, std::string const& value)
    {
        header_.append_field(field, value);
        return *this;
    }
//---------------------------------------------------------------------------------------------------------------------
    response_handler& response_handler::set(std::string const& field, std::string const& value)
    {
        header_.set_field(field, value);
        return *this;
    }
//---------------------------------------------------------------------------------------------------------------------
    response_handler& response_handler::redirect(std::string const& where, int code)
    {
        header_.set_code(code);
        header_.set_field("Location", where);
        return *this;
    }
//---------------------------------------------------------------------------------------------------------------------
    void response_handler::send(std::string const& body)
    {
        try_set("Content-Length", std::to_string(body.length()));
        try_set("Content-Type", "text/plain");

        write(this, std::make_shared <std::string> (body));
    }
//---------------------------------------------------------------------------------------------------------------------
    void response_handler::send(std::vector <char> const& body)
    {
        try_set("Content-Length", std::to_string(body.size()));
        try_set("Content-Type", "application/octet-stream");

        write(this, std::make_shared <std::vector <char>> (body));
    }
//---------------------------------------------------------------------------------------------------------------------
    void response_handler::send_status(int code)
    {
        status(code);
        if (code != 204)
            send(header_.get_message());
        else
            end();
    }
//---------------------------------------------------------------------------------------------------------------------
    void response_handler::end()
    {
        send_header([this](boost::system::error_code ec){
            connection_->get_parent()->get_connections()->remove(connection_);
            // further use of this is invalid from here.
        });
    }
//---------------------------------------------------------------------------------------------------------------------
    void response_handler::try_set(std::string const& field, std::string const& value)
    {
        if (!header_.has_field(field))
            set(field, value);
    }
//---------------------------------------------------------------------------------------------------------------------
    response_handler& response_handler::status(int code)
    {
        header_.set_code(code);
        return *this;
    }
//---------------------------------------------------------------------------------------------------------------------
    void response_handler::send_header(write_callback continuation)
    {
        if (headerSent_.load() == false)
        {
            headerSent_.store(true);
            connection_->write(header_.to_string(), continuation);
        }
        else
        {
            continuation({});
        }
    }
//---------------------------------------------------------------------------------------------------------------------
    response_handler& response_handler::type(std::string const& mime)
    {
        auto set_type = [this](std::string const& what)
        {
            if (what.empty())
                throw std::invalid_argument("could not find appropriate mime type from extension");
            header_.set_field("Content-Type", what);
        };

        if (mime.find('/') != std::string::npos)
            set_type(mime);
        else if (!mime.empty() && mime.front() == '.')
            set_type(extension_to_mime(mime));
        else
            set_type(search_mime(mime));

        return *this;
    }
//---------------------------------------------------------------------------------------------------------------------
    void response_handler::set_cookie(cookie ck)
    {
        header_.set_cookie(ck);
    }
//#####################################################################################################################
}
