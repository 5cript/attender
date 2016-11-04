#pragma once

// RFC 2616 https://tools.ietf.org/html/rfc2616#section-3.3.1

#include <string>
#include <chrono>

namespace attender
{
    class date
    {
    public:
        /**
         *  Creates a time date object from a specific time point.
         */
        date(std::chrono::system_clock::time_point time_point);

        /**
         *  Creates a time date object from "system_clock::now".
         */
        date();

        /**
         *  Returns a reference to the contained time point.
         *  Meant to be modified using STL means and ways. No need to reimplement the stuff here.
         */
        std::chrono::system_clock::time_point& get_time_point();

        /**
         *  Converts the contained time point to a time format representation specified
         *  in RFC 2616 Section 3.3.1.
         */
        std::string to_gmt_string() const;

    private:
        std::chrono::system_clock::time_point time_point_;
    };
}
