#pragma once

#include "session_cookie_generator_interface.hpp"

#include <string>

namespace attender
{
    class uuid_session_cookie_generator : public session_cookie_generator_interface
    {
    public:
        /**
         *  UUID session generator using the given options.
         *
         *  @param secure Secure cookies can only be used over SSL.
         *                Note: Never use identifying cookies without SSL.
         *  @param http_only Cookie only available to http client, not to scripts, such as JavaScript.
         *  @param max_age The maximum age of the session cookie.
         *  @param domain the domain this cookie is valid for.
         **/
        uuid_session_cookie_generator(
            bool secure = true,
            bool http_only = false,
            uint64_t max_age = {},
            std::string domain = {}
        );

        /**
         *  UUID session generator using the given options.
         *
         *  @param secure Secure cookies can only be used over SSL.
         *                Note: Never use identifying cookies without SSL.
         *  @param http_only Cookie only available to http client, not to scripts, such as JavaScript.
         *  @param max_age The maximum age of the session cookie.
         *  @param domain the domain this cookie is valid for.
         **/
        uuid_session_cookie_generator(
            bool secure = true,
            uint64_t max_age = {},
            bool http_only = false,
            std::string domain = {}
        );

    private:
        virtual std::string generate_id() const = 0;
        virtual std::string session_name() const = 0;
        virtual cookie make_cookie_base() const = 0;

    private:
        bool secure_; // false = not set
        bool http_only_; // false = not set
        uint64_t max_age_; // 0 = not set
        std::string domain_; // empty = not set
    };
}
