#pragma once

#include <attender/http/http_fwd.hpp>
#include <attender/http/cookie.hpp>

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

    /**
     * Parser and storage for http header with access methods.
     */
    class request_header
    {
    public:
        friend request_handler;

    public:
        /**
         * Returns just the path part. A request on "/bla?x=asdf" returns "/bla".
         */
        std::string get_path() const;

        /**
         * Returns the request method. "GET", "PUT", ...
         */
        std::string get_method() const;

        /**
         * Returns the whole request url part after the host. "/bla?y=asdf" returns "/bla?y=asdf".
         */
        std::string get_url() const;

        /**
         * Returns the protocol "HTTP" or "HTTPS".
         */
        std::string get_protocol() const;

        /**
         * Returns the http version. for example "1.1"
         */
        std::string get_version() const;

        /**
         * Get the entire header as a string. 
         */
        std::string to_string() const;

        /**
         * Return a header field.
         */
        boost::optional <std::string> get_field(std::string const& key) const;

        /**
         * Get a query paramter "/bla?x=2" will yield 2 for key "x".
         * @param key The query key.
         */
        boost::optional <std::string> get_query(std::string const& key) const;

        /**
         * Get cookie by name.
         * @param the name of the cookie.
         */
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
        std::string decode_url(std::string const& encoded);
        void patch_cookie(std::string const& key, std::string const& value);

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
