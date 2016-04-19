#pragma once

#include <memory>
#include <tuple>

#include <boost/fusion/algorithm/iteration/for_each.hpp>

namespace attender
{
    namespace internal
    {
        template <typename TupT>
        struct reset_all;
    }

    template <typename... List>
    class lifetime_binding
    {
    public:
        lifetime_binding(std::shared_ptr <List>... to_keep_alive)
            : kept_alive{std::move(to_keep_alive)...}
        {
        }

        void free()
        {
            boost::fusion::for_each(kept_alive, [](auto const& sptr){
                sptr->reset();
            });
        }

        lifetime_binding& operator=(lifetime_binding&&) = default;
        lifetime_binding(lifetime_binding&&) = default;

    private:
        std::tuple <std::shared_ptr <List>...> kept_alive;
    };
}
