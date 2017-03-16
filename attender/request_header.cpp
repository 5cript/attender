#include "request_header.hpp"

#include <boost/algorithm/string.hpp>

#include <regex>
#include <iostream>

namespace attender
{
//#####################################################################################################################
    std::string request_header::get_path() const
    {
        return path_;
    }
//---------------------------------------------------------------------------------------------------------------------
    std::string request_header::get_method() const
    {
        return method_;
    }
//---------------------------------------------------------------------------------------------------------------------
    std::string request_header::get_url() const
    {
        return url_;
    }
//---------------------------------------------------------------------------------------------------------------------
    std::string request_header::get_protocol() const
    {
        return protocol_;
    }
//---------------------------------------------------------------------------------------------------------------------
    std::string request_header::get_version() const
    {
        return version_;
    }
//---------------------------------------------------------------------------------------------------------------------
    boost::optional <std::string> request_header::get_field(std::string const& key) const
    {
        auto iter = fields_.find(key);
        if (iter != std::end(fields_))
            return iter->second;
        else
            return boost::none;
    }
//---------------------------------------------------------------------------------------------------------------------
    boost::optional <std::string> request_header::get_query(std::string const& key) const
    {
        auto iter = query_.find(key);
        if (iter != std::end(query_))
            return iter->second;
        else
            return boost::none;
    }
//---------------------------------------------------------------------------------------------------------------------
    boost::optional <std::string> request_header::get_cookie(std::string const& name) const
    {
        auto iter = cookies_.find(name);
        if (iter != std::end(query_))
            return iter->second;
        else
            return boost::none;
    }
//---------------------------------------------------------------------------------------------------------------------
    request_header::request_header(request_header_intermediate const& intermediate)
        : method_{intermediate.method}
        , url_{intermediate.url}
        , protocol_{intermediate.protocol}
        , version_{intermediate.version}
        , path_{}
        , fields_{intermediate.fields}
        , cookies_{intermediate.cookies}
    {
        parse_url();
    }
//---------------------------------------------------------------------------------------------------------------------
    void request_header::parse_query(std::string const& query)
    {
        std::vector <std::string> split_by_sep;
        boost::algorithm::split(split_by_sep, query, boost::algorithm::is_any_of("&;"), boost::algorithm::token_compress_off);

        for (auto const& pair : split_by_sep)
        {
            auto pos = pair.find('=');
            if (pos == std::string::npos)
                throw std::invalid_argument("invalid query format");

            query_[pair.substr(0, pos)] = pair.substr(pos + 1, pair.length() - pos - 1);
        }
    }
//---------------------------------------------------------------------------------------------------------------------
    void request_header::parse_url()
    {
        auto query_pos = url_.find_last_of('?');
        if (query_pos != std::string::npos)
        {
            parse_query(url_.substr(query_pos + 1, url_.length() - query_pos - 1));
            path_ = url_.substr(0, query_pos);
        }
        else
            path_ = url_;
    }
//#####################################################################################################################
}
