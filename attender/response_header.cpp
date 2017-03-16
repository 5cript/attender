#include "response_header.hpp"
#include "response_code.hpp"

#include <sstream>

namespace attender
{
//#####################################################################################################################
    response_header::response_header()
        : protocol_{"HTTP"}
        , version_{"2.0"}
        , code_{204}
        , message_{translate_code(code_)}
        , fields_{}
    {

    }
//---------------------------------------------------------------------------------------------------------------------
    void response_header::set_protocol(std::string const& protocol)
    {
        protocol_ = protocol;
    }
//---------------------------------------------------------------------------------------------------------------------
    void response_header::set_version(std::string const& version)
    {
        version_ = version;
    }
//---------------------------------------------------------------------------------------------------------------------
    void response_header::set_code(int code)
    {
        code_ = code;
        message_ = translate_code(code);
    }
//---------------------------------------------------------------------------------------------------------------------
    void response_header::set_message(std::string const& message)
    {
        message_ = message;
    }
//---------------------------------------------------------------------------------------------------------------------
    void response_header::set_field(std::string const& field, std::string const& value)
    {
        fields_[field] = value;
    }
//---------------------------------------------------------------------------------------------------------------------
    void response_header::append_field(std::string const& field, std::string const& value)
    {
        fields_[field] += value;
    }
//---------------------------------------------------------------------------------------------------------------------
    std::string response_header::get_protocol() const
    {
        return protocol_;
    }
//---------------------------------------------------------------------------------------------------------------------
    std::string response_header::get_version() const
    {
        return version_;
    }
//---------------------------------------------------------------------------------------------------------------------
    int response_header::get_code() const
    {
        return code_;
    }
//---------------------------------------------------------------------------------------------------------------------
    std::string response_header::get_message() const
    {
        return message_;
    }
//---------------------------------------------------------------------------------------------------------------------
    boost::optional <std::string> response_header::get_field(std::string const& key) const
    {
        auto iter = fields_.find(key);
        if (iter != std::end(fields_))
            return iter->second;
        else
            return boost::none;
    }
//---------------------------------------------------------------------------------------------------------------------
    bool response_header::has_field(std::string const& key) const
    {
        return fields_.find(key) != std::end(fields_);
    }
//---------------------------------------------------------------------------------------------------------------------
    std::string response_header::to_string() const
    {
        std::stringstream sstr;
        sstr << protocol_ << '/' << version_ << ' ' << code_ << ' ' << message_ << "\r\n";
        for (auto const& i : fields_)
        {
            sstr << i.first << ": " << i.second << "\r\n";
        }
        for (auto const& cookie : cookies_)
        {
            sstr << "Set-Cookie: " << cookie.to_set_cookie_string() << "\r\n";
        }
        sstr << "\r\n";
        return sstr.str();
    }
//---------------------------------------------------------------------------------------------------------------------
    void response_header::set_cookie(cookie const& cookie)
    {
        cookies_.push_back(cookie);
    }
//#####################################################################################################################
}
