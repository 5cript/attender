#include <attender/encoding/brotli.hpp>

#include <brotli/encode.h>

#include <iostream>
#include <cstring>

using namespace std::string_literals;

namespace attender
{
//#####################################################################################################################
    brotli_configuration brotli_configuration::make_default()
    {
        return {
            BROTLI_DEFAULT_QUALITY,
            BROTLI_DEFAULT_WINDOW,
            BROTLI_MODE_GENERIC
        };
    }
//#####################################################################################################################
    struct brotli_encoder::implementation
    {
        brotli_configuration config;
        BrotliEncoderState* ctx;

        implementation(brotli_configuration config);
        ~implementation();
    };
//---------------------------------------------------------------------------------------------------------------------
    brotli_encoder::implementation::implementation(brotli_configuration config)
        : config{std::move(config)}
    {
        ctx = BrotliEncoderCreateInstance(nullptr, nullptr, nullptr);
        BrotliEncoderSetParameter(ctx, BROTLI_PARAM_QUALITY, config.quality);
        BrotliEncoderSetParameter(ctx, BROTLI_PARAM_MODE, config.mode);
    }
//---------------------------------------------------------------------------------------------------------------------
    brotli_encoder::implementation::~implementation()
    {
        BrotliEncoderDestroyInstance(ctx);
    }
//---------------------------------------------------------------------------------------------------------------------
//#####################################################################################################################
    std::string brotli_encoder::encoding() const
    {
        return "br";
    }
//---------------------------------------------------------------------------------------------------------------------
    brotli_encoder::brotli_encoder(brotli_configuration config)
        : brotctx_{new brotli_encoder::implementation(std::move(config))}
        , input_{}
        , output_{}
        , input_start_{0}
        , output_start_offset_{0}
        , avail_in_{0}
        , input_cutoff_{128'000}
        , output_minimum_avail_{32'768}
        , output_considered_overflow_{20'000'000}
        , total_out_{0}
        , avail_{0}
        , completed_{}
        , buffer_saver_{}
    {

    }
//---------------------------------------------------------------------------------------------------------------------
    void brotli_encoder::set_minimum_avail(std::size_t min_avail)
    {
        output_minimum_avail_ = min_avail;
    }
//---------------------------------------------------------------------------------------------------------------------
    void brotli_encoder::set_cut_off(std::size_t input_cutoff)
    {
        input_cutoff_ = input_cutoff;
    }
//---------------------------------------------------------------------------------------------------------------------
    brotli_encoder::~brotli_encoder()
    {
    }
//---------------------------------------------------------------------------------------------------------------------
    std::size_t brotli_encoder::available() const
    {
        return avail_.load();
    }
//---------------------------------------------------------------------------------------------------------------------
    char const* brotli_encoder::data() const
    {
        return output_.data();
    }
//---------------------------------------------------------------------------------------------------------------------
    bool brotli_encoder::complete() const
    {
        return completed_.load();
    }
//---------------------------------------------------------------------------------------------------------------------
    void brotli_encoder::has_consumed(std::size_t size)
    {
        if (size > 0)
        {
            avail_.store(avail_.load() - size);
            std::lock_guard <std::recursive_mutex> guard{buffer_saver_};
            output_.erase(output_.begin(), output_.begin() + size);
        }
        producer::has_consumed(size);
        if (consuming_.load() == false && completed_.load())
            produced_data();
    }
//---------------------------------------------------------------------------------------------------------------------
    void brotli_encoder::on_error(boost::system::error_code)
    {
        //on_error_(ec);
    }
//---------------------------------------------------------------------------------------------------------------------
    void brotli_encoder::start_production()
    {
        //on_ready_();
        if (available() > 0)
            produced_data();
    }
//---------------------------------------------------------------------------------------------------------------------
    void brotli_encoder::flush()
    {
        do
        {
            push(nullptr, 0, BROTLI_OPERATION_FLUSH);
        } while (avail_in_ != 0);
        produced_data();
    }
//---------------------------------------------------------------------------------------------------------------------
    void brotli_encoder::finish()
    {
        do
        {
            push(nullptr, 0, BROTLI_OPERATION_FINISH);
        } while (avail_in_ != 0);
        completed_.store(true);
        produced_data();
    }
//---------------------------------------------------------------------------------------------------------------------
    void brotli_encoder::shrink_input()
    {
        if (input_.size() > input_cutoff_ && input_start_ > 0)
        {
            input_.erase(std::begin(input_), std::begin(input_) + input_start_);
            input_start_ = 0;
        }
    }
//---------------------------------------------------------------------------------------------------------------------
    void brotli_encoder::bufferize_input(char const* data_begin, std::size_t data_size)
    {
        if (avail_in_)

        shrink_input();
        input_.resize(input_.size() + data_size);

        auto start = &*input_.begin() + input_start_;
        std::memcpy(start, data_begin, data_size);
        avail_in_ += data_size;
    }
//---------------------------------------------------------------------------------------------------------------------
    void brotli_encoder::reserve_output()
    {
        std::lock_guard <std::recursive_mutex> guard(buffer_saver_);

        std::size_t avail_out = output_.size() - output_start_offset_;
        if (avail_out < output_minimum_avail_)
        {
            output_.resize(output_.size() + output_minimum_avail_);
        }
    }
//---------------------------------------------------------------------------------------------------------------------
    void brotli_encoder::push(char const* data_begin, std::size_t data_size)
    {
        push(data_begin, data_size, BROTLI_OPERATION_PROCESS);
    }
//---------------------------------------------------------------------------------------------------------------------
    void brotli_encoder::push(char const* data_begin, std::size_t data_size, int operation)
    {
        if (data_begin != nullptr && data_size > 0)
        {
            bufferize_input(data_begin, data_size);
        }
        reserve_output();

        std::lock_guard <std::recursive_mutex> guard(buffer_saver_);
        uint8_t const* next_in = &*input_.begin();
        uint8_t* next_out = reinterpret_cast <uint8_t*>(&*output_.begin() + output_start_offset_);

        std::size_t avail_out = output_.size() - output_start_offset_;
        std::size_t avail_out_before = avail_out;
        auto res = BrotliEncoderCompressStream
        (
            // state
            brotctx_->ctx,

            // operation BROTLI_OPERATION_PROCESS most of the times. type: enum BrotliEncoderOperation
            static_cast <BrotliEncoderOperation> (operation),

            // available data on the input buffer
            &avail_in_,

            // point to input (also carries OUT)
            &next_in,

            // available space in output
            &avail_out,

            // pointer to free output space
            &next_out,

            // total bytes compressed since last state initialization
            &total_out_
        );
        avail_.store(avail_.load() + (avail_out_before - avail_out));

        if (res)
            produced_data();
        else
            production_failure("error "s + std::to_string(res));
    }
//---------------------------------------------------------------------------------------------------------------------
    void brotli_encoder::buffer_locked_do(std::function <void()> const& fn) const
    {
        std::lock_guard <std::recursive_mutex> guard(buffer_saver_);
        fn();
    }
//---------------------------------------------------------------------------------------------------------------------
    void brotli_encoder::push(std::string const& data)
    {
        return push(data.data(), data.size());
    }
//---------------------------------------------------------------------------------------------------------------------
    std::size_t brotli_encoder::available_in() const noexcept
    {
        return avail_in_;
    }
//#####################################################################################################################
    brotli_encoder& operator<<(brotli_encoder& brotli, std::string const& data)
    {
        brotli.push(data);
        return brotli;
    }
//#####################################################################################################################
}
