#pragma once

#ifndef __kernel_entry
    #define __kernel_entry
#endif

#include <boost/process.hpp>
#include <boost/dll/runtime_symbol_info.hpp>
#include <filesystem>

namespace attender::testing
{
    class NodeWebSocketServer
    {
    public:
        void start(std::string const& name);
        void stop();

    private:
        std::unique_ptr<boost::process::child> node_;
    };

    void NodeWebSocketServer::start(std::string const& name)
    {
        node_ = std::make_unique<
            boost::process::child
        >(
            boost::process::search_path("node"), 
            (boost::dll::program_location().parent_path() / "node_ws" / (name + ".js")).string()
        );
    }

    void NodeWebSocketServer::stop()
    {
        node_->terminate();
    }
}