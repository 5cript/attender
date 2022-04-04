#pragma once

#include <attender/net_core.hpp>
#include <attender/io_context/async_model.hpp>

#include <vector>
#include <thread>
#include <memory>
#include <mutex>

namespace attender
{
    class thread_pooler : public async_model
    {
    public:
        thread_pooler
        (
            asio::io_context* context,
            std::size_t thread_count =
                std::thread::hardware_concurrency() == 0 ? 4 : std::thread::hardware_concurrency(),
            std::function <void()> initAction = [](){},
            std::function <void(std::exception const&)> exceptAction = [](auto const&){}
        );

        ~thread_pooler();

    protected:
        void setup_impl() override;
        void teardown_impl() override;

    private:
        asio::io_context* context_;
        std::vector <std::thread> threads_;
        std::size_t thread_count_;
        std::mutex thread_pool_lock_;
        std::unique_ptr<boost::asio::executor_work_guard<decltype(context_->get_executor())>>
            executorWorkGuard_;
        std::function <void()> initAction_;
        std::function <void(std::exception const&)> exceptAction_;
    };
}
