#pragma once

namespace attender
{
    struct settings
    {
        /** Use X-Forward-Header if trust_proxy **/
        bool trust_proxy = false;

        /** Send exceptions to client? I recommend no, to not leak information unneccessarily, but its useful for debugging, **/
        bool expose_exception = false;
    };
}
