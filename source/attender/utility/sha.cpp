#include <attender/utility/sha.hpp>

#include <openssl/evp.h>

#include <sstream>
#include <memory>
#include <functional>
#include <iomanip>

namespace attender
{
//#####################################################################################################################
    template <typename T>
    std::optional<std::string> sha(std::string_view data, T evp)
    {
        std::unique_ptr<EVP_MD_CTX, std::function<void(EVP_MD_CTX*)>> context
        {
            EVP_MD_CTX_new(),
            [](EVP_MD_CTX* ctx) {
                if (ctx != nullptr)
                    EVP_MD_CTX_free(ctx);
            }
        };

        if (!context)
            return std::nullopt;

        if(!EVP_DigestInit_ex(context.get(), evp(), nullptr))
            return std::nullopt;

        if(!EVP_DigestUpdate(context.get(), data.data(), data.length()))
            return std::nullopt;

        unsigned char hash[EVP_MAX_MD_SIZE];
        unsigned int lengthOfHash = 0;

        if(!EVP_DigestFinal_ex(context.get(), hash, &lengthOfHash))
            return std::nullopt;
        
        std::stringstream ss;
        for(unsigned int i = 0; i < lengthOfHash; ++i)
        {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
        }

        return ss.str();
    }
//---------------------------------------------------------------------------------------------------------------------
    std::optional<std::string> sha256(std::string_view data)
    {
        return sha(data, EVP_sha256);
    }
//---------------------------------------------------------------------------------------------------------------------
    std::optional<std::string> sha512(std::string_view data)
    {
        return sha(data, EVP_sha512);
    }
//#####################################################################################################################
}