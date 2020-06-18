#pragma once

#include <atomic>

namespace attender
{
    class conclusion_observer
    {
    public:
        conclusion_observer()
            : alive_{true}
            , within_endtime_{false}
        {
        }

        bool is_alive() const
        {
            return alive_.load();
        }
        bool has_concluded() const
        {
            return !alive_.load() || within_endtime_.load();
        }
        void conclude()
        {
            within_endtime_.store(true);
        }
        void has_died()
        {
            alive_.store(false);
        }

    private:
        std::atomic_bool alive_;
        std::atomic_bool within_endtime_;
    };
}
