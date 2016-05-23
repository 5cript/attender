#pragma once

#include "tcp_fwd.hpp"
#include "response_header.hpp"

namespace attender
{
    class response_handler
    {
    public:
        response_handler(std::shared_ptr <tcp_connection> connection);

        /**
         *  Appends the specified value to the HTTP response header field.
         *  If the header is not already set, it creates the header with the specified value.
         *
         *  @param field A header field identifier, such as "Warning" or "Link"
         *  @param value A value that is to be appended.
         *
         *  @return *this for chaining.
         */
        response_handler& append(std::string const& field, std::string const& value);

        /**
         *  Sets the response’s HTTP header field to value.
         *
         *  @param field A header field identifier, such as "Warning" or "Link"
         *  @param value A value that is to be appended.
         *
         *  @return *this for chaining.
         */
        response_handler& set(std::string const& field, std::string const& value);

        // TODO:
        // cookie, attachement, download, format(does not belong here), json, xml



    private:
        std::shared_ptr <tcp_connection> connection_;
        response_header header_;
    };
}
