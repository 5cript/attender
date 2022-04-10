#pragma once

#include <string>
#include <string_view>

namespace attender
{
    template <typename CharType, template <typename...> class ContainerT, typename... Dummys>
    std::string base64_encode(ContainerT <CharType, Dummys...> const& bytes)
    {
		static CharType const table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

        std::size_t b;
        std::string res;
        for (std::size_t i = 0; i < bytes.size(); i += 3)
        {
            b = (bytes[i] & 0xFC) >> 2;
            res.push_back(table[b]);
            b = (bytes[i] & 0x03) << 4;

            if (i + 1 < bytes.size())
            {
                b |= (bytes[i + 1] & 0xF0) >> 4;
                res.push_back(table[b]);
                b = (bytes[i + 1] & 0x0F) << 2;

                if (i + 2 < bytes.size())
                {
                    b |= (bytes[i + 2] & 0xC0) >> 6;
                    res.push_back(table[b]);
                    b = bytes[i + 2] & 0x3F;
                    res.push_back(table[b]);
                }
                else
                {
                    res.push_back(table[b]);
                    res.push_back('=');
                }
            }
            else
            {
                res.push_back(table[b]);
                res.push_back('=');
                res.push_back('=');
            }
        }
        return res;
    }

    template <typename CharType, template <typename...> class ContainerT, typename... Dummys>
    void base64_decode(std::string_view input, ContainerT <CharType, Dummys...>& bytes)
    {
		// static CharType const table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

        bytes.clear();

        if (input.empty())
            return;

        if (input.length() % 4 != 0)    {
            throw std::invalid_argument ("input does not have correct size for base64");
        }

        std::size_t size = (input.length() * 3) / 4;
        if (*input.rbegin() == '=')
            size--;
        if (*(input.rbegin() + 1) == '=')
            size--;

        bytes.resize(size);

        auto backwardsTable = [](int c) -> int {
            if (c >= static_cast<int>('A') && c <= static_cast<int>('Z'))
                return c - 'A';
            if (c >= static_cast<int>('a') && c <= static_cast<int>('z'))
                return c - 'a' + 26;
            if (c >= static_cast<int>('0') && c <= static_cast<int>('9'))
                return c - '0' + 52;
            if (c == '+')
                return 62;
            if (c == '/')
                return 63;
            if (c == '=')
                return 64;
            else
                throw std::invalid_argument ("input contains characters that are not base64");
            return 0;
        };

        std::size_t j = 0;
        int b[4];
        for (std::size_t i = 0; i < input.length(); i += 4)
        {
            b[0] = backwardsTable(input[i]);
            b[1] = backwardsTable(input[i + 1]);
            b[2] = backwardsTable(input[i + 2]);
            b[3] = backwardsTable(input[i + 3]);
            bytes[j++] = static_cast<CharType>((b[0] << 2) | (b[1] >> 4));
            if (b[2] < 64)
            {
                bytes[j++] = static_cast<CharType>((b[1] << 4) | (b[2] >> 2));
                if (b[3] < 64)
                {
                    bytes[j++] = static_cast<CharType>((b[2] << 6) | b[3]);
                }
            }
        }
    }
}
