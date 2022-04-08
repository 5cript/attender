#pragma once

namespace attender::websocket
{
    struct security_parameters
    {
        std::string key;
        std::string cert;
        std::string passphrase = "";
        std::string diffie_hellman_parameters = "";
    };
}