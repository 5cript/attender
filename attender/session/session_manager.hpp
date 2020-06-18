#pragma once

#include "session_storage_interface.hpp"
#include "../request.hpp"

#include <memory>
#include <boost/optional.hpp>
#include <string>

namespace attender
{
    enum class session_state
    {
        live,
        not_found,
        no_session,
        timed_out
    };

    class session_manager
    {
    public:
        session_manager(std::unique_ptr <session_storage_interface> session_storage);

        template <typename SessionT>
        session_state load_session(std::string const& session_cookie_name, SessionT* session, request_handler* req)
        {
            auto opt = req->get_cookie_value(session_cookie_name);
            if (!opt)
                return session_state::no_session;
            else
            {
                auto sessionFound = session_storage_->get_session(opt.get(), session);
                if (!sessionFound)
                    return session_state::not_found;
                return session_state::live;
            }
        }

        template <typename SessionT>
        SessionT make_session()
        {
            return SessionT{session_storage_->create_session()};
        }

        void make_session()
        {
            session_storage_->create_session();
        }

        template <typename SessionT>
        void save_session(SessionT const& session)
        {
            session_storage_->set_session(session.id(), session);
        }

        template <typename SessionT>
        void save_session(boost::optional <SessionT> const& session)
        {
            if (session)
                save_session(session.get());
        }

        template <typename SessionT>
        void terminate_session(SessionT const& session)
        {
            session_storage_->delete_session(session.id());
        }

        template <typename SessionT>
        void terminate_session(boost::optional <SessionT> const& session)
        {
            if (session)
                session_storage_->delete_session(session.get().id());
        }

        template <typename SessionStorageT>
        SessionStorageT* get_storage()
        {
            auto* ptr = session_storage_.get();
            if (ptr == nullptr)
                return nullptr;
            return dynamic_cast <SessionStorageT*> (ptr);
        }

    private:
        std::unique_ptr <session_storage_interface> session_storage_;
    };
}
