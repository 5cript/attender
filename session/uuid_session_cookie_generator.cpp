#include "uuid_session_cookie_generator.hpp"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace attender
{
//#####################################################################################################################
    uuid_session_cookie_generator::uuid_session_cookie_generator(
            bool secure,
            bool http_only,
            uint64_t max_age,
            std::string domain
    )
        : secure_{secure}
        , http_only_{http_only}
        , max_age_{max_age}
        , domain_{std::move(domain)}
    {
    }
//---------------------------------------------------------------------------------------------------------------------
    uuid_session_cookie_generator::uuid_session_cookie_generator(
        bool secure,
        uint64_t max_age,
        bool http_only,
        std::string domain
    )
        : secure_{secure}
        , http_only_{http_only}
        , max_age_{max_age}
        , domain_{std::move(domain)}
    {
    }
//---------------------------------------------------------------------------------------------------------------------
    std::string uuid_session_cookie_generator::generate_id() const
    {
        auto uuid = boost::uuids::random_generator{}();
        return boost::uuids::to_string(uuid);
    }
//---------------------------------------------------------------------------------------------------------------------
    std::string uuid_session_cookie_generator::session_name() const
    {
        return "session";
    }
//---------------------------------------------------------------------------------------------------------------------
    cookie uuid_session_cookie_generator::make_cookie_base() const
    {
        cookie keks;
        keks.set_secure(secure_)
            .set_http_only(http_only_)
            .set_max_age(max_age_)
            .set_domain(domain_)
            .set_path("/")
        ;
        return keks;
    }
//#####################################################################################################################
}
