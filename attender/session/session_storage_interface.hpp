#pragma once

#include "session.hpp"

#include <string>
#include <cstdint>

namespace attender
{
    class session_storage_interface
    {
    public:
        /**
         *  Clears all sessions from the storage.
         **/
        virtual void clear() = 0;

        /**
         *  Returns the amount of session in the session storage.
         *
         *  @attention Imlementation might be slow! Do not abuse.
         **/
        virtual uint64_t size() = 0;

        /**
         *  Create a session and return the id.
         *
         *  @warning DO NOT USE AUTOINCREMENT IDs.
         **/
        virtual std::string create_session() = 0;

        /**
         *  Used to kill a specific session.
         *
         **/
        virtual void delete_session(std::string const& id) = 0;

        /**
         *  Get a session from the storage.
         *
         *  @param id The session id.
         *  @param session [out] Some sort of session. Is ignored if session is nullptr
         *  @return Returns whether the session exists or not.
         **/
        virtual bool get_session(std::string const& id, session* session) = 0;

        /**
         *  Get a session from the storage.
         *
         *  @param id The session id.
         *  @param data Some sort of session.
         *  @return Returns whether the session exists or not.
         **/
        virtual bool set_session(std::string const& id, session const& session) = 0;

        virtual ~session_storage_interface() = default;
    };
}
