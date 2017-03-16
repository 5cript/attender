#pragma once

#include "session_storage_interface.hpp"

#include <unordered_map>

namespace attender
{
    template <typename IdGenerator, typename SessionT>
    class memory_session_storage : public session_storage_interface
    {
    public:
        void clear() override
        {
            sessions_.clear();
        }
        uint64_t size() override
        {
            return sessions_.size();
        }
        std::string create_session() override
        {
            auto id = IdGenerator::generate_id();
            sessions_[id] = SessionT{id};
            return id;
        }
        void delete_session(std::string const& id) override        {
            sessions_.erase(id);
        }
        bool get_session(std::string const& id, session& session) override
        {
            auto iter = sessions_.find(id);
            if (iter == std::end(sessions_))
                return false;
            session = iter->second;
            return true;
        }
        bool set_session(std::string const& id, session const& session) override
        {
            auto iter = sessions_.find(id);
            if (iter == std::end(sessions_))
                return false;
            iter->second = session;
            return true;
        }

    private:
        std::unordered_map <std::string, SessionT> sessions_;
    };
}
