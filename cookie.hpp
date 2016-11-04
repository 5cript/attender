#pragma once

#include "date.hpp"

#include <string>
#include <unordered_map>
#include <boost/optional.hpp>

namespace attender
{
    class cookie
    {
    public:
        /**
         *  Creates a fresh session cookie.
         *
         **/
        explicit cookie();

        /**
         *  Set cookie name.
         **/
        cookie& set_name(std::string const& name);

        /**
         *  Set cookie value.
         **/
        cookie& set_value(std::string const& value);

        /**
         *  Set cookie expiration date.
         **/
        cookie& set_expiry(date const& expires);

        /**
         *  Removes the expiration date. All cookies are session cookies after creation, so no need to call this for fresh cookies.
         **/
        cookie& make_session_cookie();

        /**
         *  Set whether or not the cookie shall be a secure cookie.
         *
         *  @param secure The cookie will only be used over HTTPS if true.
         **/
        cookie& set_secure(bool secure);

        /**
         *  Set whether or not, the cookie shall be accessible by scripting languages.
         *  This feature implemented by some browsers reduces XSS vulnerability.
         *
         *  @param http_only The cookie will not be accessible by JavaScript etc when true.
         **/
        cookie& set_http_only(bool http_only);

        /**
         *  Set the Domain attribute of the cookie.
         **/
        cookie& set_domain(std::string const& domain);

        /**
         *  Set the path on which the cookie shall be accessible from.
         **/
        cookie& set_path(std::string const& path);

        /**
         *  Returns the cookie name.
         **/
        std::string get_name() const;

        /**
         *  Returns the cookie value.
         **/
        std::string get_value() const;

        /**
         *  Returns whether the cookie is secure or not.
         **/
        bool is_secure() const;

        /**
         *  Returns whether the cookie is "HttpOnly" or not.
         **/
        bool is_http_only() const;

        /**
         *  Returns the path attribution.
         **/
        std::string get_path() const;

        /**
         *  Returns the domain attribution.
         **/
        std::string get_domain() const;

        /**
         *  Creates a set cookie string.
         **/
        std::string to_set_cookie_string() const;

    private:
        std::string name_;
        std::string value_;
        std::string domain_; // empty means not set
        std::string path_; // empty means not set
        boost::optional <date> expires_;
        bool secure_;
        bool http_only_;
    };
}
