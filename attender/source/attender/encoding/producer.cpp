#include <attender/encoding/producer.hpp>

#include <iostream>

using namespace std::chrono_literals;

namespace attender
{
    namespace
    {
#ifndef __GNUC__
        std::size_t fast_log_base2(uint64_t n)
        {
            static const int table[64] = {
                0, 58, 1, 59, 47, 53, 2, 60, 39, 48, 27, 54, 33, 42, 3, 61,
                51, 37, 40, 49, 18, 28, 20, 55, 30, 34, 11, 43, 14, 22, 4, 62,
                57, 46, 52, 38, 26, 32, 41, 50, 36, 17, 19, 29, 10, 13, 21, 56,
                45, 25, 31, 35, 16, 9, 12, 44, 24, 15, 8, 23, 7, 6, 5, 63
            };

            n |= n >> 1;
            n |= n >> 2;
            n |= n >> 4;
            n |= n >> 8;
            n |= n >> 16;
            n |= n >> 32;

            return table[(n * 0x03f6eaf2cd271461ull) >> 58ull];
        }
#endif

        std::size_t hex_size(std::size_t number)
        {
#ifdef __GNUC__
#   define LOG2(X) ((unsigned) (8*sizeof (unsigned long long) - __builtin_clzll((X)) - 1))
#else
#   define LOG2(X) fast_log_base2(uint64_t n)
#endif // __GNUC__
            if (number == 0)
                return 1ull;

            return static_cast <std::size_t> (1) + (LOG2(number) >> 2ull);
        }
    }
//#####################################################################################################################
    std::size_t producer::hexlen(std::size_t base10Number)
    {
        return hex_size(base10Number);
    }
//---------------------------------------------------------------------------------------------------------------------
    void producer::has_consumed(std::size_t)
    {
        if (available() > 0)
        {
            //std::lock_guard <std::recursive_mutex> guard{on_produce_protect_};
            if (on_produce_)
                on_produce_({}, false);
        }
        else
            consuming_.store(false);
    }
//---------------------------------------------------------------------------------------------------------------------
    void producer::set_on_produce_cb(std::function <void(std::string const& err, bool)> cb)
    {
        std::lock_guard <std::recursive_mutex> guard{on_produce_protect_};
        on_produce_ = [this, cb{std::move(cb)}](std::string const& err, bool control)
        {
            consuming_.store(true);
            cb(err, control);
        };
    }
//---------------------------------------------------------------------------------------------------------------------
    bool producer::has_consumer_attached() const
    {
        std::lock_guard <std::recursive_mutex> guard{on_produce_protect_};
        return on_produce_.operator bool();
    }
//---------------------------------------------------------------------------------------------------------------------
    void producer::produced_data() const
    {
        if (consuming_.load())
            return;

        if (has_consumer_attached())
        {
            std::lock_guard <std::recursive_mutex> guard{on_produce_protect_};
            on_produce_({}, false);
        }
    }
//---------------------------------------------------------------------------------------------------------------------
    void producer::production_failure(std::string const& fail)
    {
        on_produce_(fail, false);
    }
//---------------------------------------------------------------------------------------------------------------------
    bool producer::wait_for_consumer(std::chrono::milliseconds timeout) const
    {
        auto start_time = std::chrono::high_resolution_clock::now();
        while(!has_consumer_attached())
        {
            auto now = std::chrono::high_resolution_clock::now();
            if (now - start_time > timeout)
                return false;
            std::this_thread::sleep_for(50ms);
        }
        return true;
    }
//---------------------------------------------------------------------------------------------------------------------
    void producer::end_production(boost::system::error_code ec)
    {
        std::lock_guard <std::recursive_mutex> guard{on_produce_protect_};
        on_produce_ = {};
        on_finish_(ec);
    }
//---------------------------------------------------------------------------------------------------------------------
    void producer::set_finish_callback(std::function <void(boost::system::error_code)> const& on_finish)
    {
        on_finish_ = on_finish;
    }
//---------------------------------------------------------------------------------------------------------------------
    producer::~producer()
    {
    }
//#####################################################################################################################
}
