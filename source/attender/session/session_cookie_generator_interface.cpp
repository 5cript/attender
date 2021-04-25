#include <attender/session/session_cookie_generator_interface.hpp>

namespace attender
{
//#####################################################################################################################
    cookie session_cookie_generator_interface::create_session_cookie() const
    {
        cookie keks = make_cookie_base();
        keks.set_name(session_name())
            .set_value(generate_id());

        return keks;
    }
//---------------------------------------------------------------------------------------------------------------------
    std::string session_cookie_generator_interface::create_set_cookie_string() const
    {
        return create_session_cookie().to_set_cookie_string();
    }
//---------------------------------------------------------------------------------------------------------------------
//#####################################################################################################################
}
