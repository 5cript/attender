#include "response.hpp"
#include "tcp_connection.hpp"
#include "tcp_server.hpp"
#include "mime.hpp"

#include <boost/filesystem.hpp>

#include <iostream>
#include <fstream>
#include <memory>

namespace attender
{
//#####################################################################################################################
    /**
     *  Trick to hide template in c++ file. So that tcp_connection.hpp is not required in the header.
     */
    template <typename T>
    static void write(response_handler* repHandler, std::shared_ptr <T> data, std::function <void()> cleanup = nop)
    {
        repHandler->send_header([repHandler, data, cleanup](boost::system::error_code ec){
            // end the connection, on error
            if (ec)
            {
                cleanup();
                repHandler->end();
                return;
            }

            repHandler->get_connection()->write(*data, [repHandler, cleanup](boost::system::error_code ec){
                // end no matter what.
                cleanup();
                repHandler->end();
            });
        });
    }
//---------------------------------------------------------------------------------------------------------------------
    struct stream_keeper
    {
        stream_keeper(std::istream& stream) : stream{&stream}
        {
        }

        operator std::istream&() {return *stream;}

        std::istream* stream;
    };
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
    response_handler& response_handler::location(std::string const& where)
    {
        header_.set_field("Location", where);
        return *this;
    }
//---------------------------------------------------------------------------------------------------------------------
    response_handler& response_handler::redirect(std::string const& where, int code)
    {
        header_.set_code(code);
        return location(where);
    }
//---------------------------------------------------------------------------------------------------------------------
    void response_handler::send(std::string const& body)
    {
        try_set("Content-Length", std::to_string(body.length()));
        try_set("Content-Type", "text/plain");

        // fix code
        if (header_.get_code() == 204 && !body.empty())
            status(200);
        else if (header_.get_code() == 200 && body.empty())
            status(204);

        write(this, std::make_shared <std::string> (body));
    }
//---------------------------------------------------------------------------------------------------------------------
    void response_handler::send(std::vector <char> const& body)
    {
        try_set("Content-Length", std::to_string(body.size()));
        try_set("Content-Type", "application/octet-stream");

        // fix code
        if (header_.get_code() == 204 && !body.empty())
            status(200);
        else if (header_.get_code() == 200 && body.empty())
            status(204);

        write(this, std::make_shared <std::vector <char>> (body));
    }
//---------------------------------------------------------------------------------------------------------------------
    void response_handler::send(std::istream& body, std::function <void()> on_finish)
    {
        body.seekg(0, std::ios_base::end);
        auto size = body.tellg();
        body.seekg(0);

        try_set("Content-Length", std::to_string(size));
        try_set("Content-Type", "application/octet-stream");

        if (header_.get_code() == 204 && size > 0)
            status(200);
        else if (header_.get_code() == 200 && size == 0)
            status(204);

        write(this, std::make_shared <stream_keeper> (body), on_finish);
    }
//---------------------------------------------------------------------------------------------------------------------
    void response_handler::send_file(std::string const& fileName)
    {
        type(boost::filesystem::path{fileName}.extension().string());

        auto reader = std::make_shared <std::ifstream> (fileName, std::ios_base::binary);
        send(*reader.get(), [reader]{}); // binds the shared ptr to the function, extending the life time, until the write operation terminates.
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
