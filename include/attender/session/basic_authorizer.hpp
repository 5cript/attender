#pragma once

#include <attender/session/authorizer_interface.hpp>

#include <string_view>

namespace attender
{
    /**
     *  Do note, that this class is also incomplete.
     *  You have to provide your own user validation by deriving it.
     */
    class basic_authorizer : public authorizer_interface
    {
    public:
        virtual bool accept_authentication(std::string_view user, std::string_view password) = 0;

        void negotiate_authorization_method(request_handler* req, response_handler* res) override;
        authorization_result try_perform_authorization(request_handler* req, response_handler* res) override;
    };
}
