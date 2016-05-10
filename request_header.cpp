#include "request_header.hpp"

namespace attender
{
//#####################################################################################################################
    std::string request_header::get_path() const
    {
        if (!path_.empty())
            return path_;

        auto pos = url.find('?')
        if (pos != std::string::npos)
            path_ = url.substr(0, pos);
        else
        {
            pos = url.find('#');
            if (pos != std::string::npos)
                path_ = url.substr(0, pos);
            else
                path_ = url;
        }

        return path_;
    }
//#####################################################################################################################
}
