#include "router.hpp"
#include "request_header.hpp"

#include <boost/algorithm/string.hpp>

#include <stdexcept>
#include <deque>

// DELETE ME
#include <iostream>

namespace attender
{
//#####################################################################################################################
    path_part::path_part(std::string const& part)
        : part_(part)
        , pattern_([&part]() -> std::string {
            if (!part.empty() && part.front() == ':')
                return ".*";
            else
                return part;
        }())
    {
        using namespace std::string_literals;

        if (part == ":")
            throw std::invalid_argument("parameters need a valid name");

        // implicitly add ^ and $.
        if (!is_parameter())
        {
            if (part_.front() != '^')
                part_ = "^"s + part_;
            if (part_.back() != '$')
                part_.push_back('$');
        }
    }
//---------------------------------------------------------------------------------------------------------------------
    bool path_part::matches(std::string const& str) const
    {
        return std::regex_match(str, pattern_);
    }
//---------------------------------------------------------------------------------------------------------------------
    bool path_part::is_parameter() const
    {
        return !part_.empty() && part_.front() == ':';
    }
//---------------------------------------------------------------------------------------------------------------------
    std::string path_part::get_template() const
    {
        if (is_parameter())
            return part_.substr(1, part_.length() - 1);

        return part_;
    }
//---------------------------------------------------------------------------------------------------------------------
    std::regex path_part::get_pattern() const
    {
        return pattern_;
    }
//#####################################################################################################################
    route::route(std::string method, std::string const& path_template, connected_callback const& callback)
        : method_{std::move(method)}
        , path_parts_{}
        , callback_{callback}
    {
        initialize(path_template);
    }
//---------------------------------------------------------------------------------------------------------------------
    void route::initialize(std::string const& path_template)
    {
        std::vector <std::string> parts;
        boost::split(parts, path_template, boost::is_any_of("/"));

        for (auto i = std::begin(parts) + 1, end = std::end(parts); i < end; ++i)
            path_parts_.emplace_back(*i);
    }
//---------------------------------------------------------------------------------------------------------------------
    bool route::matches(request_header const& header) const
    {
        if (header.get_method() != method_)
            return false;

        std::deque <std::string> passed_parts;
        auto path = header.get_path();
        boost::split(passed_parts, path, boost::is_any_of("/"));
        passed_parts.pop_front();

        std::cout << passed_parts.size() << "\n";
        std::cout << path_parts_.size() << "\n";

        if (passed_parts.size() != path_parts_.size())
            return false;

        for (std::size_t i = 0u; i != path_parts_.size(); ++i)
        {
            if (!path_parts_[i].matches(passed_parts[i]))
                return false;
        }

        return true;
    }
//---------------------------------------------------------------------------------------------------------------------
    std::unordered_map <std::string, std::string> route::get_path_parameters(std::string const& path) const
    {
        std::unordered_map <std::string, std::string> result;

        std::vector <std::string> passed_parts;
        boost::split(passed_parts, path, boost::is_any_of("/"));

        for (std::size_t i = 0u; i != path_parts_.size(); ++i)
        {
            if (path_parts_[i].is_parameter())
               result[path_parts_[i].get_template()] = passed_parts[i];
        }

        return result;
    }
//---------------------------------------------------------------------------------------------------------------------
    connected_callback route::get_callback() const
    {
        return callback_;
    }
//#####################################################################################################################
    void request_router::add_route(std::string const& method, std::string const& path_template, connected_callback const& callback)
    {
        add_route({method, path_template, callback});
    }
//---------------------------------------------------------------------------------------------------------------------
    void request_router::add_route(route const& r)
    {
        routes_.push_back(r);
    }
//---------------------------------------------------------------------------------------------------------------------
    boost::optional <route> request_router::find_route(request_header const& header) const
    {
        for (auto const& i : routes_)
        {
            if (i.matches(header))
                return boost::optional <route> {i};
        }
        return boost::none;
    }
//#####################################################################################################################
}
