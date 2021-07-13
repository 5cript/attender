#pragma once

#include "unsecure_server.hpp"

#include <attender/http/response.hpp>

#include <gtest/gtest.h>
#include <iostream>

using namespace std::chrono_literals;

namespace attender::tests
{
    using client = attendee::request;

    class HttpHeaderTests : public ::testing::Test
                          , public UnsecureServer
    {
    public:
        HttpHeaderTests()
            : UnsecureServer{{}}
        {}
    };

    TEST_F(HttpHeaderTests, CanReadPath)
    {
        std::atomic_bool wasCalled{false};
        setupAndStart([this, &wasCalled](auto& server) {setupHeaderTester(server, [this, &wasCalled](auto req){
            auto header = req->get_header();
            EXPECT_EQ(header.get_path(), "/read_header");
            std::lock_guard guard{condMutex_};
            wasCalled.store(true);
        });});

        std::string url = this->url("/read_header?t=4");
        auto result = client_.get(url).perform();

        std::unique_lock guard{condMutex_};
        EXPECT_TRUE(condVar_.wait_for(guard, 1s, [&wasCalled](){return wasCalled.load();}));
    }

    TEST_F(HttpHeaderTests, CanGetMethod)
    {
        std::atomic_bool wasCalled{false};
        setupAndStart([this, &wasCalled](auto& server) {setupHeaderTester(server, [this, &wasCalled](auto req){
            auto header = req->get_header();
            EXPECT_EQ(header.get_method(), "GET");
            std::lock_guard guard{condMutex_};
            wasCalled.store(true);
        });});

        auto result = client_.get(url("/read_header")).perform();

        std::unique_lock guard{condMutex_};
        EXPECT_TRUE(condVar_.wait_for(guard, 1s, [&wasCalled](){return wasCalled.load();}));
    }

    TEST_F(HttpHeaderTests, CanGetUrl)
    {
        std::atomic_bool wasCalled{false};
        setupAndStart([this, &wasCalled](auto& server) {setupHeaderTester(server, [this, &wasCalled](auto req){
            auto header = req->get_header();
            EXPECT_EQ(header.get_url(), "/read_header?t=2");
            std::lock_guard guard{condMutex_};
            wasCalled.store(true);
        });});

        auto result = client_.get(url("/read_header?t=2")).perform();

        std::unique_lock guard{condMutex_};
        EXPECT_TRUE(condVar_.wait_for(guard, 1s, [&wasCalled](){return wasCalled.load();}));
    }

    TEST_F(HttpHeaderTests, CanGetProtocol)
    {
        std::atomic_bool wasCalled{false};
        setupAndStart([this, &wasCalled](auto& server) {setupHeaderTester(server, [this, &wasCalled](auto req){
            auto header = req->get_header();
            EXPECT_EQ(header.get_protocol(), "HTTP");
            std::lock_guard guard{condMutex_};
            wasCalled.store(true);
        });});

        auto result = client_.get(url("/read_header")).perform();

        std::unique_lock guard{condMutex_};
        EXPECT_TRUE(condVar_.wait_for(guard, 1s, [&wasCalled](){return wasCalled.load();}));
    }

    TEST_F(HttpHeaderTests, CanGetVersion)
    {
        std::atomic_bool wasCalled{false};
        setupAndStart([this, &wasCalled](auto& server) {setupHeaderTester(server, [this, &wasCalled](auto req){
            auto header = req->get_header();
            EXPECT_EQ(header.get_version(), "1.1");
            std::lock_guard guard{condMutex_};
            wasCalled.store(true);
        });});

        auto result = client_.get(url("/read_header")).perform();

        std::unique_lock guard{condMutex_};
        EXPECT_TRUE(condVar_.wait_for(guard, 1s, [&wasCalled](){return wasCalled.load();}));
    }

    TEST_F(HttpHeaderTests, CanGetQueryParameters)
    {
        std::atomic_bool wasCalled{false};
        setupAndStart([this, &wasCalled](auto& server) {setupHeaderTester(server, [this, &wasCalled](auto req){
            auto header = req->get_header();
            EXPECT_EQ(*header.get_query("x"), "2");
            EXPECT_EQ(*header.get_query("y"), "asdf");
            EXPECT_FALSE(header.get_query("notthere"));
            std::lock_guard guard{condMutex_};
            wasCalled.store(true);
        });});

        auto result = client_.get(url("/read_header?x=2&y=asdf")).perform();

        std::unique_lock guard{condMutex_};
        EXPECT_TRUE(condVar_.wait_for(guard, 1s, [&wasCalled](){return wasCalled.load();}));
    }

    TEST_F(HttpHeaderTests, CanGetFields)
    {
        std::atomic_bool wasCalled{false};
        setupAndStart([this, &wasCalled](auto& server) {setupHeaderTester(server, [this, &wasCalled](auto req){
            auto header = req->get_header();
            EXPECT_EQ(*header.get_field("Content-Type"), "application/json");
            std::lock_guard guard{condMutex_};
            wasCalled.store(true);
        });});

        auto result = client_
            .get(url("/read_header"))
            .set_header_fields({
                {"Content-Type", "application/json"}
            })
            .perform()
        ;

        std::unique_lock guard{condMutex_};
        EXPECT_TRUE(condVar_.wait_for(guard, 1s, [&wasCalled](){return wasCalled.load();}));
    }

    TEST_F(HttpHeaderTests, CanGetCookie)
    {
        std::atomic_bool wasCalled{false};
        setupAndStart([this, &wasCalled](auto& server) {setupHeaderTester(server, [this, &wasCalled](auto req){
            auto header = req->get_header();
            EXPECT_EQ(*header.get_cookie("name"), "bla");
            EXPECT_EQ(*header.get_cookie("asdf"), "23");
            EXPECT_FALSE(header.get_cookie("notthere"));
            std::lock_guard guard{condMutex_};
            wasCalled.store(true);
        });});

        auto result = client_
            .get(url("/read_header"))
            .set_header_fields({
                {"Cookie", "name=bla; asdf=23"}
            })
            .perform()
        ;

        std::unique_lock guard{condMutex_};
        EXPECT_TRUE(condVar_.wait_for(guard, 1s, [&wasCalled](){return wasCalled.load();}));
    }
}