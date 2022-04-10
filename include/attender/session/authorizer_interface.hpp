#pragma once

#include <attender/http/http_fwd.hpp>

namespace attender
{
    enum class authorization_result
    {
        allowed_continue = 0, // success, continue with actual request
        denied = 1, // improper auth. End the connection before returning this.
        negotiate = 2, // need to negotiate method first, client doesnt supply correct method. DO NOT END THE CONNECTION YOURSELF
        allowed_but_stop = 3, // success, but dont continue with request (authorizer closes the connection)
        bad_request = 4 // request is malformed. Handler closes connection
    };

    class authorizer_interface
    {
    public:
        virtual ~authorizer_interface() = default;

        /**
         *  Supply a realm. Can be anything
         */
        virtual std::string realm() const = 0;

        /**
         *  This function is only called if the client doesnt already attempt to authenticate properly.
         *  This function should respond with a WWW-Authenticate header response, so that the client knows
         *  what to do.
         **/
        virtual void negotiate_authorization_method(request_handler* req, response_handler* res) = 0;

        /**
         *  Try to authenticate user with given request.
         */
        virtual authorization_result try_perform_authorization(request_handler* req, response_handler* res) = 0;
    };
}
