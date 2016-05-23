#include "request_header.hpp"

#include <regex>

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
    std::string request_header::get_fragment() const
    {
        return fragment_;
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
    request_header::request_header(request_header_intermediate const& intermediate)
        : method_{intermediate.method}
        , url_{intermediate.url}
        , protocol_{intermediate.protocol}
        , version_{intermediate.version}
        , path_{}
        , fields_{intermediate.fields}
    {
        parse_url();
    }
//---------------------------------------------------------------------------------------------------------------------
    void request_header::parse_query(std::string const& query)
    {
        std::regex rgx( R"((\w+=(?:[\w-])+)(?:(?:&|;)(\w+=(?:[\w-])+))*)" );
        std::smatch match;

        if (std::regex_match(query, match, rgx))
        {
            for (auto i = std::begin(match) + 1; i < std::end(match); ++i)
            {
                auto pos = i->str().find_first_of('=');
                query_[i->str().substr(pos+1)] = i->str().substr(0, pos);
            }
        }
    }
//---------------------------------------------------------------------------------------------------------------------
    void request_header::parse_url()
    {
        std::regex rgx( R"((?:(?:(\/(?:(?:[a-zA-Z0-9]|[-_~!$&']|[()]|[*+,;=:@])+(?:\/(?:[a-zA-Z0-9]|[-_~!$&']|[()]|[*+,;=:@])+)*)?)|\/)?(?:(\?(?:\w+=(?:[\w-])+)(?:(?:&|;)(?:\w+=(?:[\w-])+))*))?(?:(#(?:\w|\d|=|\(|\)|\\|\/|:|,|&|\?)+))?))" );
        std::smatch match;

        if (std::regex_match(url_, match, rgx))
        {
            for (auto i = std::begin(match) + 1; i < std::end(match); ++i)
            {
                if (i->str().front() == '/')
                    path_ = i->str();
                else if (i->str().front() == '?')
                    parse_query(i->str().substr(1, i->str().length() - 1));
                else if (i->str().front() == '#')
                    fragment_ = i->str().substr(1, i->str().length() - 1);
            }
        }
        else
            throw std::invalid_argument("Not a valid url");
    }
//#####################################################################################################################
}
