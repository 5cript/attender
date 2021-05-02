#pragma once

#include "unsecure_server.hpp"

#include <attender/http/response.hpp>

#include <attendee/attendee.hpp>

#include <gtest/gtest.h>
#include <iostream>
#include <condition_variable>
#include <mutex>
#include <atomic>

using namespace std::chrono_literals;

namespace attender::tests
{
    class HttpServerTests : public ::testing::Test
                          , public UnsecureServer
    {
    public:
        HttpServerTests()
            : UnsecureServer{{}}
        {}
    };

    TEST_F(HttpServerTests, ServerIsRunningAndBoundToAPort)
    {
        setupAndStart();
        std::cout << "[          ] " << "Port: " << port_ << "\n";
        EXPECT_NE(port_, 0);
    }

    TEST_F(HttpServerTests, CanConnectAndGetSimpleUrl)
    {
        setupAndStart([this](auto& server) {setupEmpty(server);});

        auto result = client_
            .get(url("/empty"))
            .perform();

        EXPECT_EQ(result.result(), CURLE_OK);
    }

    TEST_F(HttpServerTests, EmptyRequestReturns204)
    {
        setupAndStart([this](auto& server) {setupEmpty(server);});

        auto result = client_
            .get(url("/empty"))
            .perform();

        EXPECT_EQ(result.result(), CURLE_OK);
        EXPECT_EQ(result.code(), 204);
    }

    TEST_F(HttpServerTests, MissingRouteReturns404)
    {
        setupAndStart([this](auto&) {});

        auto result = client_
            .get(url("/invalid"))
            .perform();
    
        EXPECT_EQ(result.code(), 404);
    }
}