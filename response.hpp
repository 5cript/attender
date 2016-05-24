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
         *  Sets the response code.
         *
         *  @param code A response code 1xx, 2xx, 3xx, 4xx or 5xx
         *
         *  @return *this for chaining.
         */
        response_handler& status(int code);

        /**
         *  Performs a mime type lookup and sets the Content-Type to the appropriate value.
         *  If the string contains a slash, it will just take the parameter "mime" as the type.
         *  If the string starts with a dot, it will try to interpret it as a file extension.
         *  Otherwise, it will try to find it in a list.
         *  This function will throw if no mime type could be matched.
         *  Please do note, that the convenience for writing "png" instead of "image/png" results
         *  in a much higher computational cost.
         *
         *  @param mime A type or a file extension. Examples:
         *              .html -> text/html
         *              html -> text/html
         *              png -> image/png
         *              application/json -> application/json
         *              json -> application/json
         *
         *  @return *this for chaining.
         */
        response_handler& type(std::string const& mime);

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
         *  This function will set the status and send the status
         *  message a string in the body.
         *  The code must be supported / known to attender.
         *
         *  Content-Length will automatically be set, if not previously defined.
         *  Content-Type will be set to "text" for this overload.
         *
         *  @param code A response code 1xx, 2xx, 3xx, 4xx or 5xx
         */
        void send_status(int code);

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
