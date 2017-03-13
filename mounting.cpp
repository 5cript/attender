#include "mounting.hpp"
#include "response.hpp"

namespace attender
{
//#####################################################################################################################
    mount_response& mount_response::append(std::string const& field, std::string const& value)
    {
        header_.append_field(field, value);
        return *this;
    }
//---------------------------------------------------------------------------------------------------------------------
    mount_response& mount_response::set(std::string const& field, std::string const& value)
    {
        header_.set_field(field, value);
        return *this;
    }
//---------------------------------------------------------------------------------------------------------------------
    mount_response& mount_response::try_set(std::string const& field, std::string const& value)
    {
        if (!header_.has_field(field))
            set(field, value);
        return *this;
    }
//---------------------------------------------------------------------------------------------------------------------
    void mount_response::to_response(response_handler& res) const
    {
        res.header_ = header_;
    }
//#####################################################################################################################
}
