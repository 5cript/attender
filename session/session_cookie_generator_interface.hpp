#pragma once

#include "../cookie.hpp"

#include <string>

namespace attender
{
    class session_cookie_generator_interface
    {
    public:
        /**
         *  Crates a cookie using the implemented functions of the NVI.
         **/
        cookie create_session_cookie() const;

        /**
         *  Crates a cookie using the implemented functions of the NVI and directly converts it into a string.
         **/
        std::string create_set_cookie_string() const;

    public:
        virtual ~session_cookie_generator_interface() = default;

    private:
        virtual std::string generate_id() const = 0;
        virtual std::string session_name() const = 0;
        virtual cookie make_cookie_base() const = 0;
    };
}
