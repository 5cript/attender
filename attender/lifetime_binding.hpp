#pragma once

#include "tcp_fwd.hpp"

#include <memory>
#include <tuple>

#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/fusion/algorithm/iteration/for_each.hpp>

namespace attender
{
    /**
     *  Internal lifetime binder that combines the lifetime of the response and request object.
     */
    class lifetime_binding
    {
    public:
        lifetime_binding(request_handler* req, response_handler* res);
        ~lifetime_binding();

        lifetime_binding(lifetime_binding const&) = delete;
        lifetime_binding& operator=(lifetime_binding const&) = delete;
        lifetime_binding& operator=(lifetime_binding&&) = delete;
        lifetime_binding(lifetime_binding&&) = delete;

        request_handler& get_request_handler();
        response_handler& get_response_handler();

    private:
        std::unique_ptr <request_handler> req_;
        std::unique_ptr <response_handler> res_;
    };
}
