#pragma once

#include "tcp_fwd.hpp"
#include "response_header.hpp"
#include "cookie.hpp"
#include "encoding/producer.hpp"

#include <atomic>

namespace attender
{
    /**
     *  The response_handler is for everything writing and sending related.
     */
    class response_handler
    {
        friend mount_response;

    public:
        explicit response_handler(tcp_connection_interface* connection) noexcept;
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
         *  @param no_except Does not throw on failure, but instead fails silently by doing nothing.
         *
         *  @return *this for chaining.
         */
        response_handler& type(std::string const& mime, bool no_except = false);

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
         *  Sends the HTTP response. After a call to send, the status and header fields
         *  can no longer be changed as they will be sent with this function.
         *  As this function completes the response, chaining will no longer be possible.
         *  THE STREAM MUST BE SEEKABLE (some boost iostreams do not).
         *  THE STREAM MUST SURVIVE FOR THE ENTIRE CONNECTION.
         *
         *  Content-Length will automatically be set, if not previously defined.
         *
         *  @param body A body to send.
         *  @param on_finish A callback function, that is called after the send operation finished.
         */
        void send(std::istream& body, std::function <void()> const& on_finish = nop);

        /**
         *  Sends the HTTP response. After a call to send, the status and header fiels
         *  can no longer be changed as they will be sent with this function.
         *  As this function completes the response, chaining will no longer be possible.
         *
         *  Content-Length will automatically be set, if not previously defined.
         *  Will force set Transfer-Encoding to chunked.
         *
         *  @param body A body to send.
         *  @param on_finish A callback function that is called after the send operation finished.
         *                   This function CANNOT determine the prod.complete(), because its only called
         *                   when the production already completed.
         */
        void send_chunked
        (
            producer& prod,
            std::function <void(boost::system::error_code)> const& on_finish = [](auto){}
        );

        /**
         *  Sends the HTTP response. After a call to send, the status and header fields
         *  can no longer be changed as they will be sent with this function.
         *  As this function completes the response, chaining will no longer be possible.
         *  The stream must provide seek and tell (some boost iostreams do not).
         *
         *  Content-Length will automatically be set, if not previously defined.
         *  Content-Type will be deduced from the filename if possible, "application/octet-stream" otherwise.
         *
         *  @param fileName A file to open in binary read mode and send.
         *  @return Returns false if the file could not be opened. The connection will not be closed and nothing will be sent.
         */
        bool send_file(std::string const& fileName);

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
         *  Sets the location http header value to the specified path value.
         *
         *  @param where The location to be set in the location header.
         */
        response_handler& location(std::string const& where);

        /**
         *  Sets the HTTP Link response header entry.
         *
         *  @param links a map that maps rel (see documentation of Link header entry) to an url.
         */
        response_handler& links(std::map <std::string /* rel */, std::string /* url */> const& links);

        /**
         *  Performs a redirect to a different url or path.
         *  This only sets the code and "Location" header field.
         *  This still has to be completed with an end statement (send, end)
         *
         *  @param where Where to redirect to.
         *  @param code A response code to send. Defaults to 302.
         */
        response_handler& redirect(std::string const& where, int code = 302);

        /**
         *  Ends the response process.
         *  Use to quickly end the response without any data. If you need to respond with data,
         *  instead use methods such as send and json.
         *  As this function completes the response, chaining will no longer be possible.
         *
         *  Do not call this function or any for that matter, after an exception was thrown or
         *  the request got finalized by a previous end call. The call to end invalidates all
         *  functions on request_handler, response_handler and the tcp_connection itself.
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

        /**
         *  Will set a cookie.
         **/
        void set_cookie(cookie ck);

        /**
         *  Returns whether or not sending or ending this is still possible.
         *  Useful for mount returns.
         */
        bool has_concluded() const;

        // TODO:
        // cookie, attachement, download, format(does not belong here), json, xml

    private:
        /**
         *  Like set, but only sets the field, if not provided earlier.
         */
        void try_set(std::string const& field, std::string const& value);

    private:
        tcp_connection_interface* connection_;
        response_header header_;
        std::atomic_bool headerSent_;
    };
}
