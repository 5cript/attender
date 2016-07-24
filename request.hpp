#pragma once

#include "tcp_fwd.hpp"
#include "request_header.hpp"
#include "request_parser.hpp"
#include "callback_wrapper.hpp"
#include "tcp_connection_interface.hpp"

#include <iosfwd>
#include <boost/thread/future.hpp>
#include <unordered_map>

namespace attender
{
//#####################################################################################################################
    class request_handler
    {
        friend tcp_server;

    public:
        request_handler(tcp_connection_interface* connection);
        ~request_handler();

        /**
         *  Returns the request header.
         *
         *  @return A http request header.
         */
        request_header get_header() const;

        callback_wrapper& read_body(std::ostream& stream);

        /**
         *  Contains the hostname derived from the Host HTTP header.
         *  When the trust proxy setting does not evaluate to false,
         *  this property will instead have the value of the X-Forwarded-Host header field.
         *  This header can be set by the client or by the proxy.
         *
         *  @return Returns host name from header.
         */
        std::string hostname() const;

        /**
         *  Contains the remote IP address of the request.
         *  This ip is taken from the system.
         *
         *  @return remote ip.
         */
        std::string ip() const;

        /**
         *  Returns a string corresponding to the HTTP method of the request: GET, POST, PUT, and so on.
         *
         *  @return request method / verb.
         */
        std::string method() const;

        /**
         *  Returns the original request url.
         *
         *  @return request url.
         */
        std::string url() const;

        /**
         *  Returns parsed path parameters by key.
         *  e.g.: /api/:param1/:param2, :param1 and :param2 are the parameters.
         *
         *  @param key A path parameter key. Leading colon can be omitted.
         *
         *  @return Returns the path part that contains the key.
         */
        std::string param(std::string const& key) const;

        /**
         *  Contains the path part of the request URL.
         *
         *  @return Returns a path, such as /api/path
         */
        std::string path() const;

        /**
         *  Returns the underlying protocol.
         *  Will allways be 'http', unless TLS is used, in which case it will be 'https'
         */
        std::string protocol() const;

        /**
         *  Retrieves a query value for a given key.
         */
        boost::optional <std::string> query(std::string const& key) const;

        /**
         *  Will return whether this is an encrypted connection or not.
         */
        bool secure() const;

        /**
         *  Returns a header field from the request header.
         *  e.g.: get_header_field("Host") -> "localhost".
         *
         *  @param key The header fields key.
         *
         *  @return The Header fields value or boost::none.
         */
        boost::optional <std::string> get_header_field(std::string const& key) const;

    private:
        // read handlers
        void header_read_handler(boost::system::error_code ec);
        void body_read_handler(boost::system::error_code ec);

    private:
        // internals
        uint64_t get_content_length() const;

    private:
        // befriended
        void initiate_header_read(parse_callback on_parse);
        void set_parameters(std::unordered_map <std::string, std::string> const& params);

    private:
        request_parser parser_;
        request_header header_;
        tcp_connection_interface* connection_;
        std::shared_ptr <tcp_read_sink> sink_;
        parse_callback on_parse_;
        std::unordered_map <std::string, std::string> params_;
        callback_wrapper on_finished_read_;
    };
//#####################################################################################################################
}
