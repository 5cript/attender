#pragma once

#ifndef __kernel_entry
    #define __kernel_entry
#endif

#include <boost/process.hpp>
#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/algorithm/string/split.hpp>

#include <filesystem>
#include <string_view>
#include <stdexcept>
#include <unordered_map>
#include <iostream>

namespace attender::testing
{
    class NodeWebSocketServer
    {
    public:
        using readCbType = std::function<void(boost::system::error_code const& ec, std::unordered_map <std::string, std::string> const&)>;

        void start(std::string const& name, boost::asio::io_service& ios, bool secure);
        void stop();
        void read(readCbType const& onRead);

    private:
        static std::unordered_map <std::string, std::string> parseMessage(std::string_view view);

    private:
        std::unique_ptr<boost::process::child> node_;
        std::unique_ptr<boost::process::async_pipe> asyncPipe_;
        std::vector<char> readBuf_;
    };

    void NodeWebSocketServer::start(std::string const& name, boost::asio::io_service& ios, bool secure)
    {
        asyncPipe_ = std::make_unique<boost::process::async_pipe>(ios);
        readBuf_.resize(4096);

        node_ = std::make_unique<
            boost::process::child
        >(
            boost::process::search_path("node"), 
            (boost::dll::program_location().parent_path() / "node_ws" / (name + ".js")).string(),
            secure ? "--secure=true" : "--secure=false",
            boost::process::std_out > *asyncPipe_
        );
        if (!node_->running())
        {
            throw std::runtime_error("node is not running");
        }
    }

    void NodeWebSocketServer::read(readCbType const& onRead)
    {
        asyncPipe_->async_read_some(boost::asio::buffer(readBuf_), [onRead, this](auto const& ec, std::size_t size) 
        {
            onRead(ec, NodeWebSocketServer::parseMessage(std::string_view{readBuf_.data(), size}));
        });
    }

    std::unordered_map <std::string, std::string> NodeWebSocketServer::parseMessage(std::string_view view)
    {
        if (!view.starts_with("<ctrl>:"))
        {
            std::cout << view << "\n";
            return {};
        }

        view.remove_prefix(7);
        view.remove_suffix(view.length() - view.find('\n'));

        std::vector <std::string> seps;
        boost::algorithm::split(seps, view, boost::is_any_of(";"));

        std::unordered_map <std::string, std::string> kv;
        for (auto const& prop : seps)
        {
            std::vector <std::string> keyValue;
            boost::algorithm::split(keyValue, prop, boost::is_any_of("="));
            if (keyValue.size() != 2)
                continue;

            kv[keyValue[0]] = keyValue[1];
        }
        return kv;
    }

    void NodeWebSocketServer::stop()
    {
        if (node_)
        {
            node_->terminate();
        }
    }
}