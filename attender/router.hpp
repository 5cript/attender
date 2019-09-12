#pragma once

#include "tcp_fwd.hpp"
#include "mounting.hpp"

#include <regex>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>

#include <boost/optional.hpp>

namespace attender
{
//#####################################################################################################################
    enum class match_result
    {
        no_match,
        path_match, // no METHOD match
        full_match
    };
//#####################################################################################################################
    class path_part
    {
    public:
        path_part(std::string const& part);

        bool is_parameter() const;
        std::string get_template() const;
        std::string get_part() const;
        std::regex get_pattern() const;

        bool matches(std::string const& str) const;

    private:
        std::string part_;
        std::regex pattern_;
    };
//#####################################################################################################################
    class route
    {
    public:
        route(std::string method, std::string const& path_template, connected_callback const& callback, bool mount_route = false);
        match_result matches(request_header const& header) const;
        std::unordered_map <std::string, std::string> get_path_parameters(std::string const& path) const;
        connected_callback get_callback() const;

    private:
        void initialize(std::string const& path_template);

    private:
        std::string method_;
        std::vector <path_part> path_parts_;
        connected_callback callback_;
        bool mount_route_;
    };
//#####################################################################################################################
    /**
     *  The request router maps paths to handler functions.
     */
    class request_router
    {
    public:
        void add_route(std::string const& method, std::string const& path_template, connected_callback const& callback, int priority = 0);
        void add_route(route const& r, int priority = 0);
        void mount(
            std::string const& root_path,
            std::string const& path_template,
            mount_callback const& callback,
            mount_option_set const& supported_methods
        );

        void mount(
            std::string const& root_path,
            std::string const& path_template,
            mount_callback_2 const& callback,
            mount_option_set const& supported_methods,
            int priority = -100
        );

        boost::optional <route> find_route(request_header const& header, match_result& match_level) const;

    private:
        std::multimap <int, route, std::greater<int>> routes_;
    };
//#####################################################################################################################
}
