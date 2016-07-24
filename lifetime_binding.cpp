#include "lifetime_binding.hpp"

#include "request.hpp"
#include "response.hpp"

namespace attender
{
//#####################################################################################################################
    lifetime_binding::lifetime_binding(request_handler* req, response_handler* res)
        : req_{req}
        , res_{res}
    {

    }
//---------------------------------------------------------------------------------------------------------------------
    lifetime_binding::~lifetime_binding() = default;
//#####################################################################################################################
}
