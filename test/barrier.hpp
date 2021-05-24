#pragma once

#include <mutex>
#include <condition_variable>
#include <chrono>
#include <atomic>

class Barrier
{
public:
    bool halt(std::chrono::milliseconds maxWait)
    {
        if (stale_)
            throw std::runtime_error("dont reuse a barrier");
        stale_ = true;
        condition_ = false;
        std::unique_lock lock{mut_};
        bool res = condVar_.wait_for(lock, maxWait, [this](){ return condition_.load();});
        condition_.store(true);
        return res;
    }

    template <typename FunctionT>
    void pass(FunctionT const& secureDo)
    {
        {
            std::scoped_lock lock{mut_};
            if (!condition_.load())
                secureDo();
            condition_ = true;
        }
        condVar_.notify_one();
    }

private:
    std::atomic_bool stale_{false};
    std::atomic_bool condition_{false};
    std::mutex mut_{};
    std::condition_variable condVar_{};
};