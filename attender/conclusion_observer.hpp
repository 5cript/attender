#pragma once

#include <atomic>

namespace attender
{
    class conclusion_observer
    {
    public:
        conclusion_observer()
            : alive_{true}
        {
        }

        bool is_unconcluded() const
        {
            return alive_.load();
        }
        bool has_concluded() const
        {
            return !alive_.load();
        }
        void conclude()
        {
            alive_.store(false);
        }

    private:
        std::atomic_bool alive_;
    };
}
