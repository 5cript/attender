#pragma once

#include "../net_core.hpp"
#include "async_model.hpp"

#include <vector>
#include <thread>
#include <memory>
#include <mutex>

namespace attender
{
    class thread_pooler : public async_model
    {
    public:
        thread_pooler(asio::io_service* context,
                      std::size_t thread_count = std::thread::hardware_concurrency() == 0
                                                 ? 4
                                                 : std::thread::hardware_concurrency()
        );

        ~thread_pooler();

    protected:
        void setup_impl() override;
        void teardown_impl() override;

    private:
        asio::io_service* context_;
        std::vector <std::thread> threads_;
        std::size_t thread_count_;
        std::mutex thread_pool_lock_;
        std::unique_ptr <boost::asio::io_service::work> work_;
    };
}
