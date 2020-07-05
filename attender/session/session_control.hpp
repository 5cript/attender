#pragma once

#include "session_storage_interface.hpp"
#include "authorizer_interface.hpp"

#include "../tcp_fwd.hpp"
#include "../cookie.hpp"

#include <string>
#include <memory>
#include <functional>

namespace attender
{
    struct SessionControlParam
    {
        std::unique_ptr <session_storage_interface>&& session_storage;
        std::unique_ptr <authorizer_interface>&& authorizer;
        std::string const& id_cookie_key;
        bool allowOptionsUnauthorized = false;
        cookie const& cookie_base = cookie{};
        std::function <void(request_handler*, response_handler*)> authorization_conditioner = {};
        bool disable_automatic_authentication = false;
        std::string authentication_path; // used if disable_automatic_authentication is true.
    };

    struct SessionControl
    {
        std::shared_ptr <session_manager> sessions;
        std::shared_ptr <authorizer_interface> authorizer;
        std::function <void(request_handler*, response_handler*)> authorization_conditioner;
        std::string id_cookie_key;
        bool allowOptionsUnauthorized;
        cookie cookie_base;
        bool disable_automatic_authentication;
        std::string authentication_path;
    };
}
