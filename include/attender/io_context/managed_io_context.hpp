#pragma once

#include <attender/net_core.hpp>
#include <attender/io_context/async_model.hpp>

#include <memory>

namespace attender
{
    template <typename AsyncModel>
    class managed_io_context
    {
    public:
        template <typename... Forwards>
        constexpr managed_io_context(Forwards&&... args)
            : context_{}
            , async_provider_{new AsyncModel{&context_, std::forward <Forwards&&>(args)...}}
        {
            async_provider_->setup();
        }

        /**
         *  Returns the encapsulated io_context, for use.
         *  The managed_io_context does not get transferred to anything else ever,
         *  its just there to combine the io_context with some work / workers.
         *  As boost::asio supports thread pools, coroutines, etc.
         */
        constexpr asio::io_context* get_io_context() noexcept
        {
            return &context_;
        }

        void teardown() { async_provider_->teardown(); }

        managed_io_context(const managed_io_context&) = delete;
        managed_io_context& operator=(const managed_io_context&) = delete;
    private:
        asio::io_context context_;
        std::unique_ptr <AsyncModel> async_provider_;
    };
}
