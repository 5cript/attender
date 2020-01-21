#pragma once

#include <memory>

namespace attender
{
    struct brotli_configuration
    {
        int quality;
        int window;
        int mode;

        static brotli_encoder make_default();
    };

    class brotli_encoder : public producer
    {
    public:
        brotli_encoder(brotli_configuration = brotli_configuration::make_default());
        ~brotli_encoder();

        /// return 'br'
        std::string encoding() const override;

        /**
         *  Sets the amount of bytes where the buffer is started to be trimmed.
         *  Stay well within cachable ranges. definitely less than 512kb.
         */
        void set_cut_off(std::size_t input_cutoff);

        /**
         *  Sets the amount of bytes that the output buffer should have available.
         *  Do NOT call while operation is in progress.
         */
        void set_minimum_avail(std::size_t min_avail);

        /// Do NOT call concurrently with itself or other pushes!
        void push(char const* data_begin, std::size_t data_size);

        /// Do NOT call concurrently with itself or other pushes!
        void push(std::string const& data);

        brotli_encoder& operator<<(brotli_encoder& brotli, std::string const& data);

        // derived methods
        std::size_t available() const override;
        char const* data() const override;
        bool complete() const override;
        void has_consumed(std::size_t size) override;
        void on_error(boost::system::error_code) override;
        void start_production() override;

    private:
        void shrink_input();
        void bufferize_input(char const* data_begin, std::size_t data_size);
        void reserve_output();
        void process();

    private:
        struct implementation;
        std::unique_ptr <Implementation> brotctx_;

        std::vector <uint8_t> input_;
        std::vector <char> output_;
        std::size_t input_start_;
        std::size_t output
        std::size_t avail_in_;
        std::size_t input_cutoff_;
        std::size_t output_minimum_avail_;

        std::atomic <std::size_t> avail_;
        std::atomic_bool completed_;
        std::recursive_mutex buffer_saver_;
    };
}
