#pragma once

#include <memory>
#include <tuple>

#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/fusion/algorithm/iteration/for_each.hpp>

namespace attender
{
    template <typename... List>
    class lifetime_binding
    {
    public:
        lifetime_binding(std::shared_ptr <List>... to_keep_alive)
            : kept_alive{std::move(to_keep_alive)...}
        {
        }

        lifetime_binding& operator=(lifetime_binding&&) = delete;
        lifetime_binding(lifetime_binding&&) = delete;

    private:
        std::tuple <std::shared_ptr <List>...> kept_alive;
    };
}
