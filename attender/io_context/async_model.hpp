#pragma once

#include <atomic>

namespace attender
{
    class async_model
    {
    public:
        /**
         *  Every model has to be tore down on destruction.
         */
        virtual ~async_model() = default;

        async_model() = default;

        // all async models are not copyable
        async_model(async_model const&) = delete;
        async_model& operator=(async_model const&) = delete;

        void setup();
        void teardown();

    protected:
        /**
         *  Setups the async_model. Must be overridden by the deriving class.
         *  The subclass can decide to call this by default on construction.
         *  There may be instances however, where setup on construction is undesirable, or
         *  impossible.
         */
        virtual void setup_impl() = 0;


        /**
         *  Tears down the internals.
         */
        virtual void teardown_impl() = 0;

    private:
        std::atomic_bool up_;
    };
}
