#include <attender/session/basic_authorizer.hpp>
#include <attender/encoding/base64.hpp>
#include <attender/http/request.hpp>
#include <attender/http/response.hpp>

using namespace std::string_literals;

namespace attender
{
//#####################################################################################################################
    void basic_authorizer::negotiate_authorization_method(request_handler*, response_handler* res)
    {
        res->append
        (
            "WWW-Authenticate",
            "Basic realm=\""s + realm() + "\", charset=\"UTF-8\""
        );
        res->status(401).end();
    }
//---------------------------------------------------------------------------------------------------------------------
    authorization_result basic_authorizer::try_perform_authorization(request_handler* req, response_handler* res)
    {
        auto maybeAuth = req->get_header_field("Authorization");
        if (!maybeAuth)
            return authorization_result::negotiate;

        // Extract info from auth header.
        auto auth = maybeAuth.value();
        auto pos = auth.find(' ');
        if (pos == std::string::npos || pos == auth.size() - 1)
        {
            res->status(400).send("Authorization header field is malformed");
            return authorization_result::bad_request;
        }

        auto type = std::string_view(auth.c_str(), pos);
        auto value = std::string_view(auth.c_str() + pos + 1, auth.size() - pos - 1);

        // Wrong method, negotiate right one.
        if (type != "Basic")
            return authorization_result::negotiate;

        std::string out;
        try
        {
            base64_decode(value, out);
        }
        catch(...)
        {
            res->status(400).send("Invalid base64 in Authorization");
            return authorization_result::bad_request;
        }
        auto colonPos = out.find(':');
        if (colonPos == std::string::npos)
        {
            res->status(400).send("Decoded base64 does not contain colon - empty passwords still need it");
            return authorization_result::bad_request;
        }

        auto user = std::string_view(out.c_str(), colonPos);
        auto pw = std::string_view(out.c_str() + colonPos + 1, out.size() - colonPos - 1);

        auto do_accept = accept_authentication(user, pw);
        if (do_accept)
            return authorization_result::allowed_continue;
        else
        {
            res->status(401).end();
            return authorization_result::denied;
        }
    }
//#####################################################################################################################
}
