#include "brotli.hpp"

#include <brotli/encode.h>

namespace attender
{
//#####################################################################################################################
    struct brotli_encoder::implementation
    {
        brotli_configuration config;
        BrotliEncoderState* ctx;

        implementation(brotli_configuration config);
        ~implementation();
    };
//---------------------------------------------------------------------------------------------------------------------
    implementation::implementation(brotli_configuration config)
        : config{std::move(config)}
    {
        ctx = BrotliEncoderCreateInstance(nullptr, nullptr, nullptr);
        BrotliEncoderSetParameter(ctx, BROTLI_PARAM_QUALITY, config.quality);
        BrotliEncoderSetParameter(ctx, BROTLI_PARAM_MODE, config.mode);
    }
//---------------------------------------------------------------------------------------------------------------------
    implementation::~implementation()
    {
        BrotliEncoderDestroyInstance(ctx);
    }
//---------------------------------------------------------------------------------------------------------------------
    brotli_encoder make_default()
    {
        return {
            BROTLI_DEFAULT_QUALITY,
            BROTLI_DEFAULT_WINDOW,
            BROTLI_MODE_GENERIC
        }
    }
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
        , avail_in_{0}
        , input_cutoff_{128'000}
        , output_minimum_avail_{32'768}
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
        return buffer_.data();
    }
//---------------------------------------------------------------------------------------------------------------------
    bool brotli_encoder::complete() const
    {
        return completed_.load();
    }
//---------------------------------------------------------------------------------------------------------------------
    void brotli_encoder::has_consumed(std::size_t size)
    {
        avail_.store(avail_.load() - size);
        std::lock_guard <std::recursive_mutex> guard{buffer_saver_};
        buffer_.erase(buffer_.begin(), buffer_.begin() + size);

        producer::has_consumed(size);
    }
//---------------------------------------------------------------------------------------------------------------------
    void brotli_encoder::on_error(boost::system::error_code)
    {
        //on_error_(ec);
    }
//---------------------------------------------------------------------------------------------------------------------
    void brotli_encoder::start_production()
    {
        on_ready_();
        if (available() > 0)
            produced_data();
    }
//---------------------------------------------------------------------------------------------------------------------
    void brotli_encoder::process()
    {

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
        shrink_input();
        input_.resize(input_.size() + data_size);

        auto start = &*input_.begin() + input_start_;
        std::memcpy(start, data_begin, data_size);
        avail_in_ += data_size;
    }
//---------------------------------------------------------------------------------------------------------------------
    void brotli_encoder::push(char const* data_begin, std::size_t data_size)
    {
        bufferize_input(data_begin, data_size);

        uint8_t const* next_in = &*input_.begin();

        std::lock_guard <std::recursive_mutex> guard(buffer_saver_);
        uint8_t const* next_out = &*input_.begin();
        BrotliEcnoderCompressStream
        (
            // state
            brotctx_.impl_->ctx,

            // operation - compress, compression does not need to be completed here.
            BROTLI_OPERATION_PROCESS,

            // available data on the input buffer
            &avail_in_,

            // point to input (also carries OUT)
            &next_in,


    }
//---------------------------------------------------------------------------------------------------------------------
    void brotli_encoder::push(std::string const& data)
    {
        push(data.data(), data.size());
    }
//#####################################################################################################################
    brotli_encoder& operator<<(brotli_encoder& brotli, std::string const& data)
    {

    }
//#####################################################################################################################
}
