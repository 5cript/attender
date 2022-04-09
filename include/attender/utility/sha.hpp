#pragma once

#include <string>
#include <string_view>
#include <optional>

namespace attender
{
    std::optional<std::string> sha256(std::string_view data);
    std::optional<std::string> sha512(std::string_view data);
}