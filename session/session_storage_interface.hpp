#pragma once

#include <string>
#include <cstdint>

namespace attender
{
    class session_storage_interface
    {
    public:
        /**
         *  Used to kill a specific session.
         *
         **/
        virtual void destroy(std::string const& id) = 0;

        /**
         *  Clears all sessions from the storage.
         **/
        virtual void clear() = 0;

        /**
         *  Returns the amount of session in the session storage.
         **/
        virtual uint64_t size() = 0;




        virtual ~session_storage_interface() = default;
    };
}
