#pragma once

#include <functional>
#include <atomic>

namespace attender
{
    class callback_wrapper
    {
    public:
        callback_wrapper()
            : func_{}
            , fail_{}
        {
        }

        /**
         *  Sets a continuation function.
         */
        template <typename FunctionT>
        callback_wrapper& then(FunctionT const& f)
        {
            if (fullfilled_.load())
                func_();
            else
                func_ = f;

            return *this;
        }

        /**
         *  Sets a continuation function.
         */
        template <typename FunctionT>
        callback_wrapper& then(FunctionT&& f)
        {
            if (fullfilled_.load())
                func_();
            else
                func_ = std::move(f);

            return *this;
        }

        /**
         *  Sets an error callback.
         */
        template <typename FunctionT>
        callback_wrapper& except(FunctionT const& f)
        {
            if (error_.load())
                fail_(last_ec_.load());
            else
                fail_ = f;

            return *this;
        }

        /**
         *  Sets an error callback.
         */
        template <typename FunctionT>
        callback_wrapper& except(FunctionT&& f)
        {
            if (error_.load())
                fail_(last_ec_.load());
            else
                fail_ = std::move(f);

            return *this;
        }

        /**
         *  Fullfill the "promise-like" callback wrapper.
         *  Will either call func_ or set a flag, that
         *  then calls the function immediately.
         */
        void fullfill()
        {
            fullfilled_.store(true);

            if (func_)
                func_();
        }

        /**
         *  Same as fullfill, but instead of calling the continuation function,
         *  it will call the error handler function with the ec.
         */
        void error(boost::system::error_code const& ec)
        {
            if (fail_)
                fail_(ec);
            else
                last_ec_.store(ec);

            error_.store(true);
        }

    private:
        std::function <void()> func_;
        std::function <void(boost::system::error_code)> fail_;
        std::atomic_bool fullfilled_;
        std::atomic_bool error_;
        std::atomic <boost::system::error_code> last_ec_;
    };
}
