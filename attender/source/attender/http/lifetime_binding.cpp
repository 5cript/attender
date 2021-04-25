#include <attender/http/lifetime_binding.hpp>

#include <attender/http/request.hpp>
#include <attender/http/response.hpp>

namespace attender
{
//#####################################################################################################################
    lifetime_binding::lifetime_binding(request_handler* req, response_handler* res)
        : req_{req}
        , res_{res}
    {

    }
//---------------------------------------------------------------------------------------------------------------------
    request_handler& lifetime_binding::get_request_handler()
    {
        return *req_.get();
    }
//---------------------------------------------------------------------------------------------------------------------
    response_handler& lifetime_binding::get_response_handler()
    {
        return *res_.get();
    }
//---------------------------------------------------------------------------------------------------------------------
    lifetime_binding::~lifetime_binding() = default;
//#####################################################################################################################
}
