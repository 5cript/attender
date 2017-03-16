#pragma once

#include "cookie.hpp"

#include <boost/optional.hpp>

#include <string>
#include <unordered_map>

namespace attender
{
    struct request_header_intermediate
    {
        std::string method;
        std::string url;
        std::string protocol;
        std::string version;

        std::unordered_map <std::string, std::string> fields;
        std::unordered_map <std::string, std::string> cookies;
    };

    class request_header
    {
    public:
        std::string get_path() const;
        std::string get_method() const;
        std::string get_url() const;
        std::string get_protocol() const;
        std::string get_version() const;
        std::string get_fragment() const;

        boost::optional <std::string> get_field(std::string const& key) const;
        boost::optional <std::string> get_query(std::string const& key) const;
        boost::optional <std::string> get_cookie(std::string const& name) const;

        request_header() = default;

        request_header(request_header_intermediate const& intermediate);

        request_header(request_header const&) = default;
        request_header(request_header&&) = default;
        request_header& operator=(request_header const&) = default;
        request_header& operator=(request_header&&) = default;

    private:
        void parse_query(std::string const& query);
        void parse_url();

    private:
        std::string method_;
        std::string url_;
        std::string protocol_;
        std::string version_;
        std::string path_;

        std::unordered_map <std::string, std::string> fields_;
        std::unordered_map <std::string, std::string> query_;
        std::unordered_map <std::string, std::string> cookies_;
    };
}
