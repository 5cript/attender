#include "response_header.hpp"

namespace attender
{
//#####################################################################################################################
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
//#####################################################################################################################
}
