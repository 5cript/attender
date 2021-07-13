#include <attender/io_context/async_model.hpp>

namespace attender
{
//#####################################################################################################################
    void async_model::setup()
    {
        if (up_.load())
            return; // already up

        up_.store(true);
        setup_impl();
    }
//---------------------------------------------------------------------------------------------------------------------
    void async_model::teardown()
    {
        if (!up_.load())
            return; // not up

        up_.store(false);
        teardown_impl();
    }
//#####################################################################################################################
}
