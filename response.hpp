#pragma once

#include "tcp_fwd.hpp"
#include "response_header.hpp"

#include <atomic>

namespace attender
{
    class response_handler
    {
    public:
        using shared_connection = std::shared_ptr <tcp_connection_interface>;

    public:
        response_handler(std::shared_ptr <tcp_connection> connection);
        ~response_handler();

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

        /**
         *  Sets the response code. code must be valid/known.
         *
         *  @param code A response code 1xx, 2xx, 3xx, 4xx or 5xx
         *
         *  @return *this for chaining.
         */
        response_handler& status(int code);

        /**
         *  Sends the HTTP response. After a call to send, the status and header fields
         *  can no longer be changed as they will be sent with this function.
         *  As this function completes the response, chaining will no longer be possible.
         *
         *  Content-Length will automatically be set, if not previously defined.
         *  Content-Type will be set to "text/plain" for this overload.
         *
         *  @param body A body to send.
         */
        void send(std::string const& body);

        /**
         *  Sends the HTTP response. After a call to send, the status and header fields
         *  can no longer be changed as they will be sent with this function.
         *  As this function completes the response, chaining will no longer be possible.
         *
         *  Content-Length will automatically be set, if not previously defined.
         *  Content-Type will be set to "application/octet-stream" for this overload.
         *
         *  @param body A body to send.
         */
        void send(std::vector <char> const& body);

        /**
         *  Ends the response process.
         *  Use to quickly end the response without any data. If you need to respond with data,
         *  instead use methods such as send and json.
         *  As this function completes the response, chaining will no longer be possible.
         */
        void end();

        /**
         *  Do NOT use this function!
         *  Returns a handle to the underlying connection.
         */
        tcp_connection_interface* get_connection();

        /**
         *  Do NOT use this function!
         *  Will send the header and set the "headerSent_" flag.
         */
        void send_header(write_callback continuation);

        // TODO:
        // cookie, attachement, download, format(does not belong here), json, xml

    private:
        /**
         *  Like set, but only sets the field, if not provided earlier.
         */
        void try_set(std::string const& field, std::string const& value);

    private:
        shared_connection connection_;
        response_header header_;
        std::atomic_bool headerSent_;
    };
}
