#pragma once

#include <attender/http/cookie.hpp>

#include <boost/optional.hpp>

#include <string>
#include <vector>
#include <unordered_map>

namespace attender
{
    class response_header
    {
    public:
        response_header();

        // default behaviour for big 5
        response_header(response_header const&) = default;
        response_header(response_header&&) = default;
        response_header& operator=(response_header const&) = default;
        response_header& operator=(response_header&&) = default;
        ~response_header() = default;

        void set_protocol(std::string const& protocol);
        void set_version(std::string const& version);
        void set_code(int code);
        void set_message(std::string const& message);
        void set_field(std::string const& field, std::string const& value);
        void append_field(std::string const& field, std::string const& value);
        void set_cookie(cookie const& cookie);

        std::string get_protocol() const;
        std::string get_version() const;
        int get_code() const;
        std::string get_message() const;
        boost::optional <std::string> get_field(std::string const& key) const;
        bool has_field(std::string const& key) const;

        std::string to_string() const;

    private:
        std::string protocol_; // example: HTTP
        std::string version_; // example: 2.0
        int code_; // example: 200
        std::string message_; // example: OK
        std::unordered_map <std::string, std::string> fields_; // example: Content-Length: 138
        std::vector <cookie> cookies_;
    };
}
