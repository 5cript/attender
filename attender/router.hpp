#pragma once

#include "tcp_fwd.hpp"
#include "mounting.hpp"

#include <regex>
#include <string>
#include <vector>
#include <unordered_map>

#include <boost/optional.hpp>

namespace attender
{
//#####################################################################################################################
    class path_part
    {
    public:
        path_part(std::string const& part);

        bool is_parameter() const;
        std::string get_template() const;
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
        route(std::string method, std::string const& path_template, connected_callback const& callback);
        bool matches(request_header const& header) const;
        std::unordered_map <std::string, std::string> get_path_parameters(std::string const& path) const;
        connected_callback get_callback() const;

    private:
        void initialize(std::string const& path_template);

    private:
        std::string method_;
        std::vector <path_part> path_parts_;
        connected_callback callback_;
    };
//#####################################################################################################################
    class request_router
    {
    public:
        void add_route(std::string const& method, std::string const& path_template, connected_callback const& callback);
        void add_route(route const& r);
        void mount(
            std::string const& root_path,
            std::string const& path_template,
            mount_callback const& callback,
            mount_option_set const& supported_methods
        );

        boost::optional <route> find_route(request_header const& header) const;

    private:
        std::vector <route> routes_;
    };
//#####################################################################################################################
}
