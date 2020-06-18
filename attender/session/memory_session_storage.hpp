#pragma once

#include "session_storage_interface.hpp"

#include <unordered_map>
#include <mutex>

namespace attender
{
    template <typename IdGenerator, typename SessionT>
    class memory_session_storage : public session_storage_interface
    {
    public:
        void clear() override
        {
            std::lock_guard <std::mutex> guard{lock_};
            sessions_.clear();
        }
        uint64_t size() override
        {
            std::lock_guard <std::mutex> guard{lock_};
            return sessions_.size();
        }
        std::string create_session() override
        {
            std::lock_guard <std::mutex> guard{lock_};
            auto id = gen_.generate_id();
            sessions_[id] = SessionT{id};
            return id;
        }
        void delete_session(std::string const& id) override        {
            std::lock_guard <std::mutex> guard{lock_};
            sessions_.erase(id);
        }
        bool get_session(std::string const& id, session* session) override
        {
            std::lock_guard <std::mutex> guard{lock_};
            auto iter = sessions_.find(id);
            if (iter == std::end(sessions_))
                return false;
            if (session != nullptr)
                *static_cast <SessionT*> (session) = iter->second;
            return true;
        }
        bool set_session(std::string const& id, session const& session) override
        {
            std::lock_guard <std::mutex> guard{lock_};
            auto iter = sessions_.find(id);
            if (iter == std::end(sessions_))
                return false;
            iter->second = session;
            return true;
        }

    private:
        std::unordered_map <std::string, SessionT> sessions_;
        IdGenerator gen_;
        std::mutex lock_;
    };
}
