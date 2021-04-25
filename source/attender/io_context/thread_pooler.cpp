#include <attender/io_context/thread_pooler.hpp>

namespace attender
{
//#####################################################################################################################
    thread_pooler::thread_pooler
    (
        asio::io_service* context,
        std::size_t thread_count,
        std::function <void()> initAction,
        std::function <void(std::exception const&)> exceptAction
    )
        : async_model{}
        , context_{context}
        , threads_{}
        , thread_count_{thread_count}
        , work_{nullptr}
        , initAction_{std::move(initAction)}
        , exceptAction_{std::move(exceptAction)}
    {

    }
//---------------------------------------------------------------------------------------------------------------------
    thread_pooler::~thread_pooler()
    {
        teardown();
    }
//---------------------------------------------------------------------------------------------------------------------
    void thread_pooler::setup_impl()
    {
        std::lock_guard <std::mutex> guard(thread_pool_lock_);

        work_ = std::make_unique <boost::asio::io_service::work> (*context_);

        for (auto i = thread_count_; i; --i)
        {
            threads_.push_back(std::thread{
                [this]{
                    if (initAction_)
                        initAction_();
                    try
                    {
                        context_->run();
                    }
                    catch(std::exception const& exc)
                    {
                        if (exceptAction_)
                            exceptAction_(exc);
                    }
                    catch(...)
                    {
                        if (exceptAction_)
                            exceptAction_(std::runtime_error{"some unknown exception was caught to prevent thread crash to kill program"});
                    }
                }
            });
        }
    }
//---------------------------------------------------------------------------------------------------------------------
    void thread_pooler::teardown_impl()
    {
        std::lock_guard <std::mutex> guard(thread_pool_lock_);

        work_.reset(nullptr);

        for (auto& thread : threads_)
        {
            if (thread.joinable())
                thread.join();
        }
        threads_.clear();
    }
//#####################################################################################################################
}
