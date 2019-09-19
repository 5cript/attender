#include "router.hpp"
#include "request_header.hpp"
#include "response.hpp"
#include "request.hpp"

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include <stdexcept>
#include <deque>
#include <fstream>
#include <iostream>

namespace attender
{
//#####################################################################################################################
    path_part::path_part(std::string const& part)
        : part_(part)
        , pattern_([&part]() -> std::string {
            if (!part.empty() && part.front() == ':')
                return ".*";
            else {
                // implicitly add ^ and $.
                using namespace std::string_literals;
                auto p = part;
                if (p.front() != '^')
                    p = "^"s + p;
                if (p.back() != '$')
                    p.push_back('$');
                return p;
            }
        }())
    {
        if (part == ":")
            throw std::invalid_argument("parameters need a valid name");
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
//---------------------------------------------------------------------------------------------------------------------
    std::string path_part::get_part() const
    {
        return part_;
    }
//#####################################################################################################################
    route::route(std::string method, std::string const& path_template, connected_callback const& callback, bool mount_route)
        : method_{std::move(method)}
        , path_parts_{}
        , callback_{callback}
        , mount_route_{mount_route}
    {
        initialize(path_template);
    }
//---------------------------------------------------------------------------------------------------------------------
    void route::initialize(std::string const& path_template)
    {
        if (!mount_route_)
        {
            std::vector <std::string> parts;
            boost::split(parts, path_template, boost::is_any_of("/"));

            for (auto i = std::begin(parts) + 1, end = std::end(parts); i < end; ++i)
                path_parts_.emplace_back(*i);
        }
        else
        {
            path_parts_.push_back(path_template);
        }
    }
//---------------------------------------------------------------------------------------------------------------------
    match_result route::matches(request_header const& header) const
    {
        if (!mount_route_)
        {
            std::deque <std::string> passed_parts;
            auto path = header.get_path();
            boost::split(passed_parts, path, boost::is_any_of("/"));
            passed_parts.pop_front();

            if (passed_parts.size() != path_parts_.size())
                return match_result::no_match;

            for (std::size_t i = 0u; i != path_parts_.size(); ++i)
            {
                if (!path_parts_[i].matches(passed_parts[i]))
                    return match_result::no_match;
            }

            if (header.get_method() != method_)
                return match_result::path_match;
            else
                return match_result::full_match;
        }
        else
        {
            auto path = header.get_path();
            auto part = path_parts_[0].get_part();
            if (path.length() < part.length())
                return match_result::no_match;
            for (std::string::size_type i = 0, end = part.length(); i != end; ++i)
                if (part[i] != path[i])
                    return match_result::no_match;

            if (header.get_method() != method_)
                return match_result::path_match;
            else
                return match_result::full_match;
        }
    }
//---------------------------------------------------------------------------------------------------------------------
    std::unordered_map <std::string, std::string> route::get_path_parameters(std::string const& path) const
    {
        std::unordered_map <std::string, std::string> result;

        std::deque <std::string> passed_parts;
        boost::split(passed_parts, path, boost::is_any_of("/"));
        passed_parts.pop_front();

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
    void request_router::add_route(std::string const& method, std::string const& path_template, connected_callback const& callback, int priority)
    {
        add_route({method, path_template, callback}, priority);
    }
//---------------------------------------------------------------------------------------------------------------------
    void request_router::add_route(route const& r, int priority)
    {
        routes_.emplace(std::make_pair(priority, r));
    }
//---------------------------------------------------------------------------------------------------------------------
    void request_router::mount(
        std::string const& root_path,
        std::string const& path_template,
        mount_callback const& callback,
        mount_option_set const& supported_methods
    )
    {
        mount(root_path, path_template, [cb = callback](request_handler* request, response_handler* mount_response, std::string_view)
        {
            return cb(request, mount_response);
        }, supported_methods);
    }
//---------------------------------------------------------------------------------------------------------------------
    void request_router::mount(
        std::string const& root_path,
        std::string const& path_template,
        mount_callback_2 const& callback,
        mount_option_set const& supported_methods,
        int priority
    )
    {
        if (supported_methods.empty())
            throw std::invalid_argument("supported methods must not be empty.");

        #define WRAP(...) \
            __VA_ARGS__

        #define MOUNT_CASE_BEGIN_BASE(METHOD, CAPTURE_BOX) \
        case (mount_options::METHOD): \
        { \
            add_route({#METHOD, path_template, \
            [CAPTURE_BOX, cutFromFront, root_path](auto req, auto res) { \
            auto path = req->path(); \
            path.erase(0, cutFromFront); \
            if (root_path.back() != '/' && root_path.back() != '\\' && path.front() != '/') \
                path = root_path + "/" + path; \
            else \
                path = root_path + path; \
            if (callback(req, res, path)) \
            {

        #define MOUNT_CASE_BEGIN(METHOD) \
            MOUNT_CASE_BEGIN_BASE(METHOD, callback)

        #define MOUNT_CASE_BEGIN_CAPTURE(METHOD, CAPTURES) \
            MOUNT_CASE_BEGIN_BASE(METHOD, WRAP(callback, CAPTURES))

        #define MOUNT_CASE_END() \
            } \
            else if (!res->has_concluded()) \
            { \
                res->send_status(403); \
            } \
            }, true}, priority); \
            break; \
        }

        auto cutFromFront = path_template.length();

        for (auto const& method : supported_methods) switch (method)
        {
            MOUNT_CASE_BEGIN(GET)
            {
                if (!validate_path(req->path()))
                    res->send_status(403);
                else
                {
                    if (!res->send_file(path))
                        res->send_status(404);
                }
            }
            MOUNT_CASE_END()
            //------------------------------------------------------
            MOUNT_CASE_BEGIN(PUT)
            {
                if (!validate_path(req->path()))
                    res->send_status(403);
                else
                {
                    std::shared_ptr <std::ofstream> writer(new std::ofstream{path, std::ios_base::binary});
                    if (!writer->good())
                        res->status(400).send(path + " not openable");
                    else
                        req->read_body(*writer, 0).then([writer, res](){
                            res->status(204).end();
                        }).except([](auto){
                            //std::cout << err.message() << "\n";
                        });
                }
            }
            MOUNT_CASE_END()
            //------------------------------------------------------
            MOUNT_CASE_BEGIN(POST)
            {
                if (!validate_path(req->path()))
                    res->send_status(403);
                else
                {
                    std::shared_ptr <std::ofstream> writer(new std::ofstream{path, std::ios_base::binary});
                    if (!writer->good())
                        res->status(400).send(path + " not openable");
                    else
                        req->read_body(*writer, 0).then([writer, res](){
                            res->send_status(204);
                        }).except([](auto){

                        });
                }
            }
            MOUNT_CASE_END()
            //------------------------------------------------------
            MOUNT_CASE_BEGIN(DELETE)
            {
                if (!validate_path(req->path()))
                    res->send_status(403);
                else
                {
                    boost::filesystem::remove_all(path);
                    res->send_status(204);
                }
            }
            MOUNT_CASE_END()
            //------------------------------------------------------
            MOUNT_CASE_BEGIN(HEAD)
            {
                if (!validate_path(req->path()))
                    res->send_status(403);
                else
                {
                    std::ifstream reader(path, std::ios_base::binary);
                    if (!reader.good())
                        res->send_status(404);
                }
            }
            MOUNT_CASE_END()
            //------------------------------------------------------
            MOUNT_CASE_BEGIN_CAPTURE(OPTIONS, supported_methods)
            {
                bool beg = true;
                std::string allowed_methods;
                for (auto const& i : supported_methods)
                {
                    if (!beg)
                        allowed_methods += ", ";
                    allowed_methods += mount_option_to_string(i);
                    beg = false;
                }
                res->set("Allow", allowed_methods);
                res->send_status(200);
            }
            MOUNT_CASE_END()
        }
    }
//---------------------------------------------------------------------------------------------------------------------
    boost::optional <route> request_router::find_route(request_header const& header, match_result& match_level) const
    {
        match_result best_result = match_result::no_match;
        for (auto const& [prio, i] : routes_)
        {
            match_level = i.matches(header);
            if (match_level == match_result::path_match)
                best_result = match_result::path_match;
            else if (match_level == match_result::full_match)
                return boost::optional <route> {i};
        }
        match_level = best_result;
        return boost::none;
    }
//#####################################################################################################################
}
