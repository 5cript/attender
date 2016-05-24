#pragma once

#include <string>

namespace attender
{
    /**
     *  Converts a file extension to a mime type if known.
     *  @param extension A file extension including dot in front.
     *
     *  @return a MIME type.
     */
    std::string extension_to_mime(std::string const& extension);

    /**
     *  Trys to find a substring of a mime and return the mime type.
     */
    std::string search_mime(std::string const& part);
}
