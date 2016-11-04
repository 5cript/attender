#include "date.hpp"

#include <sstream>
#include <iomanip>

namespace attender
{
//#####################################################################################################################
    std::string tm_formatter(std::tm* tm, const char* suffix = nullptr)
    {
        static constexpr const char* weekdays[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
        static constexpr const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

        std::stringstream sstr;
        sstr << weekdays[tm->tm_wday] << ", "
             << tm->tm_mday << " "
             << months[tm->tm_mon] << ' '
             << tm->tm_year << " "
             << std::setfill('0') << std::setw(2) << tm->tm_hour << ':'
             << tm->tm_min << ':'
             << tm->tm_sec
        ;
        if (suffix != nullptr)
            sstr << suffix;
        return sstr.str();
    }
//#####################################################################################################################
    date::date(std::chrono::system_clock::time_point time_point)
        : time_point_{std::move(time_point)}
    {
    }
//---------------------------------------------------------------------------------------------------------------------
    date::date()
        : time_point_{std::chrono::system_clock::now()}
    {
    }
//---------------------------------------------------------------------------------------------------------------------
    std::chrono::system_clock::time_point& date::get_time_point()
    {
        return time_point_;
    }
//---------------------------------------------------------------------------------------------------------------------
    std::string date::to_gmt_string() const
    {
        auto time = std::chrono::system_clock::to_time_t(time_point_);
        return (gmtime(&time), " GMT");
    }
//#####################################################################################################################
}
