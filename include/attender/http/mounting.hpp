#pragma once

#include <attender/http/http_fwd.hpp>
#include <attender/http/mount_options.hpp>
#include <attender/http/response_header.hpp>

namespace attender
{
    /**
     *  A mount response is like a response_handler, but has much less functions.
     */
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
         *  Sets the response�s HTTP header field to value.
         *
         *  @param field A header field identifier, such as "Warning" or "Link"
         *  @param value A value that is to be appended.
         *
         *  @return *this for chaining.
         */
        mount_response& set(std::string const& field, std::string const& value);

        /**
         *  Sets the response�s HTTP header field to value, if not set
         *
         *  @param field A header field identifier, such as "Warning" or "Link"
         *  @param value A value that is to be appended.
         *
         *  @return *this for chaining.
         */
        mount_response& try_set(std::string const& field, std::string const& value);

        /**
         *  Sets the response code.
         *
         *  @param code A response code 1xx, 2xx, 3xx, 4xx or 5xx
         *
         *  @return *this for chaining.
         */
        mount_response& status(int status);

        /**
         *  Gets the response code.
         *
         *  @return A response code 1xx, 2xx, 3xx, 4xx or 5xx
         */
        int get_status() const;

        void to_response(response_handler& res) const;

    private:
        response_header header_;
        int status_ = 0;
    };

    bool validate_path(std::string const& str);
}
