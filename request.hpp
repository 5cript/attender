#pragma once

#include "tcp_fwd.hpp"
#include "request_header.hpp"
#include "request_parser.hpp"

#include <iosfwd>

namespace attender
{
    class readable_request_handler;

//#####################################################################################################################
    class request_handler
    {
        friend readable_request_handler;

    public:
        request_handler(std::shared_ptr <tcp_connection> connection);

        /**
         *  Returns the request header.
         *
         *  @return A http request header.
         */
        request_header get_header() const;

        void initiate_header_read(parse_callback on_parse);

        readable_request_handler initiate_read(read_callback callback);

    private:
        // read handlers
        void header_read_handler(boost::system::error_code ec);
        uint64_t get_content_length() const;

    private:
        request_parser parser_;
        std::shared_ptr <tcp_connection> connection_;
        std::shared_ptr <tcp_read_sink> sink_;
        parse_callback on_parse_;
    };
//#####################################################################################################################
    class readable_request_handler
    {
        friend request_handler;

    public:
        void read_body(std::ostream& stream);

    private:
        readable_request_handler(request_handler* parent, read_callback callback);
        void body_read_handler(boost::system::error_code ec);

    private:
        request_handler* res_;
        read_callback callback_;
    };
//#####################################################################################################################
}
