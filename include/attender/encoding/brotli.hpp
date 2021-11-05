#pragma once

#include "producer.hpp"

#include <memory>
#include <vector>
#include <atomic>
#include <mutex>

namespace attender
{
    struct brotli_configuration
    {
        int quality;
        int window;
        int mode;

        static brotli_configuration make_default();
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

        /**
         *  Returns the amount of available bytes in the input buffer.
         */
        std::size_t available_in() const noexcept;

        /**
         * Do NOT call concurrently with itself or other pushes!
         */
        void push(char const* data_begin, std::size_t data_size);

        /**
         * Do NOT call concurrently with itself or other pushes!
         */
        void push(std::string const& data);

        friend brotli_encoder& operator<<(brotli_encoder& brotli, std::string const& data);

        // derived methods
        std::size_t available() const override;
        char const* data() const override;
        bool complete() const override;
        void has_consumed(std::size_t size) override;
        void on_error(boost::system::error_code) override;
        void start_production() override;
        void buffer_locked_do(std::function <void()> const&) const  override;

        /// Flushes the compressed data to output. Warning not the same as finish.
        void flush();

        /// Flushes remaining data to output and completes the compression.
        void finish();

        // requires data to have ".data()" and ".size()" and be convertible to char const*
        template <typename T>
        friend brotli_encoder& operator<<(brotli_encoder& stream, T const& data)
        {
            stream.push(data.data(), data.size());
            return stream;
        }

        friend brotli_encoder& operator<<(brotli_encoder& stream, char const* nullterminated)
        {
            return operator<<(stream, std::string_view{nullterminated});
        }

        template <typename T>
        friend std::enable_if_t <std::is_integral_v <T>, brotli_encoder&> operator<<(brotli_encoder& stream, T integral)
        {
            return operator<<(stream, std::to_string(integral));
        }

    private:
        void shrink_input();
        void bufferize_input(char const* data_begin, std::size_t data_size);
        void reserve_output();
        void push(char const* data_begin, std::size_t data_size, int operation);

    private:
        struct implementation;
        std::unique_ptr <implementation> brotctx_;

        std::vector <uint8_t> input_;
        std::vector <char> output_;
        std::size_t input_start_;

        /// Offset where brotli can write INTO the buffer
        std::size_t output_start_offset_;
        std::size_t avail_in_;
        std::size_t input_cutoff_;
        std::size_t output_minimum_avail_;
        std::size_t output_considered_overflow_;
        std::size_t total_out_;

        std::atomic <std::size_t> avail_;
        std::atomic_bool completed_;
        mutable std::recursive_mutex buffer_saver_;
    };
}
