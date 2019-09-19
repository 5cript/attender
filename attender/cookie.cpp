#include "cookie.hpp"

#include <sstream>
#include <boost/algorithm/string.hpp>

namespace attender
{
//#####################################################################################################################
    cookie::cookie()
        : name_{}
        , value_{}
        , domain_{}
        , path_{}
        , expires_{}
        , max_age_{0}
        , secure_{}
        , http_only_{false}
    {

    }
//---------------------------------------------------------------------------------------------------------------------
    cookie::cookie(std::string const& name, std::string const& value)
        : name_{name}
        , value_{value}
        , domain_{}
        , path_{}
        , expires_{}
        , max_age_{0}
        , secure_{}
        , http_only_{false}
    {

    }
//---------------------------------------------------------------------------------------------------------------------
    std::unordered_map <std::string, std::string> cookie::parse_cookies(std::string const& cookie_header_entry)
    {
        using namespace std::string_literals;

        if (cookie_header_entry.empty())
            throw std::runtime_error("cookie value is empty");

        std::vector <std::string> split;
        boost::algorithm::split(split, cookie_header_entry, boost::algorithm::is_any_of(";"), boost::algorithm::token_compress_on);

        std::unordered_map <std::string, std::string> result;
        for (auto iter = std::begin(split), end = std::end(split); iter != end; ++iter)
        {
            auto eqpos = iter->find('=');
            if (eqpos == std::string::npos)
                throw std::runtime_error("cookie name value pair does not contain '=' character");

            auto left = boost::algorithm::trim_left_copy(iter->substr(0, eqpos));
            auto right = iter->substr(eqpos + 1, iter->size() - eqpos - 1);

            result[left] = right;
        }
        return result;
    }
//---------------------------------------------------------------------------------------------------------------------
    cookie& cookie::set_name(std::string const& name)
    {
        name_ = name;
        return *this;
    }
//---------------------------------------------------------------------------------------------------------------------
    cookie& cookie::set_value(std::string const& value)
    {
        value_ = value;
        return *this;
    }
//---------------------------------------------------------------------------------------------------------------------
    cookie& cookie::set_expiry(date const& expires)
    {
        expires_ = expires;
        return *this;
    }
//---------------------------------------------------------------------------------------------------------------------
    cookie& cookie::make_session_cookie()
    {
        expires_ = boost::none;
        return *this;
    }
//---------------------------------------------------------------------------------------------------------------------
    cookie& cookie::set_secure(bool secure)
    {
        secure_ = secure;
        return *this;
    }
//---------------------------------------------------------------------------------------------------------------------
    cookie& cookie::set_http_only(bool http_only)
    {
        http_only_ = http_only;
        return *this;
    }
//---------------------------------------------------------------------------------------------------------------------
    cookie& cookie::set_domain(std::string const& domain)
    {
        domain_ = domain;
        return *this;
    }
//---------------------------------------------------------------------------------------------------------------------
    cookie& cookie::set_path(std::string const& path)
    {
        path_ = path;
        return *this;
    }
//---------------------------------------------------------------------------------------------------------------------
    cookie& cookie::set_max_age(uint64_t age)
    {
        max_age_ = age;
        return *this;
    }
//---------------------------------------------------------------------------------------------------------------------
    std::string cookie::get_name() const
    {
        return name_;
    }
//---------------------------------------------------------------------------------------------------------------------
    std::string cookie::get_value() const
    {
        return value_;
    }
//---------------------------------------------------------------------------------------------------------------------
    bool cookie::is_secure() const
    {
        return secure_;
    }
//---------------------------------------------------------------------------------------------------------------------
    bool cookie::is_http_only() const
    {
        return http_only_;
    }
//---------------------------------------------------------------------------------------------------------------------
    std::string cookie::get_path() const
    {
        return path_;
    }
//---------------------------------------------------------------------------------------------------------------------
    std::string cookie::get_domain() const
    {
        return domain_;
    }
//---------------------------------------------------------------------------------------------------------------------
    uint64_t cookie::get_max_age() const
    {
        return max_age_;
    }
//---------------------------------------------------------------------------------------------------------------------
    std::string cookie::to_set_cookie_string() const
    {
        std::stringstream sstr;
        sstr << name_ << "=" << value_;

        if (!domain_.empty())
            sstr << "; Domain=" << domain_;
        if (!path_.empty())
            sstr << "; Path=" << path_;
        if (max_age_ > 0)
            sstr << "; Max-Age=" << max_age_;
        if (expires_)
            sstr << "; Expires=" << expires_.get().to_gmt_string();
        if (secure_)
            sstr << "; Secure";
        if (http_only_)
            sstr << "; HttpOnly";

        return sstr.str();
    }
//#####################################################################################################################
}
