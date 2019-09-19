#pragma once

#include <unordered_set>

#ifdef DELETE
#undef DELETE
#endif // DELETE

namespace attender
{
    enum mount_options
    {
        GET,
        PUT,
        POST,
        DELETE,
        HEAD,
        OPTIONS
    };

    using mount_option_set = std::unordered_set <mount_options, std::hash <int>>;

    const char* mount_option_to_string(mount_options option);
}
