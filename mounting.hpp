#pragma once

#include "tcp_fwd.hpp"
#include "mount_options.hpp"
#include "response_header.hpp"

namespace attender
{
    class mount_response
    {
    public:
        /**
         *  Appends the specified value to the HTTP response header field.
         *  If the header is not already set, it creates the header with the specified value.
         *
         *  @param field A header field identifier, such as "Warning" or "Link"
         *  @param value A value that is to be appended.
         *
         *  @return *this for chaining.
         */
        mount_response& append(std::string const& field, std::string const& value);

        /**
         *  Sets the response’s HTTP header field to value.
         *
         *  @param field A header field identifier, such as "Warning" or "Link"
         *  @param value A value that is to be appended.
         *
         *  @return *this for chaining.
         */
        mount_response& set(std::string const& field, std::string const& value);

        /**
         *  Sets the response’s HTTP header field to value, if not set
         *
         *  @param field A header field identifier, such as "Warning" or "Link"
         *  @param value A value that is to be appended.
         *
         *  @return *this for chaining.
         */
        mount_response& try_set(std::string const& field, std::string const& value);

        void to_response(response_handler& res) const;

    private:
        response_header header_;
    };

    bool validate_path(std::string const& str);
}
