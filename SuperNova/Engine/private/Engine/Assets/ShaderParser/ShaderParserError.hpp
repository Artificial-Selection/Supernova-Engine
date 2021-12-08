#pragma once

#include <stdexcept>


namespace snv
{

class ShaderParserError : public std::runtime_error
{
public:
    explicit ShaderParserError(const char* what)
        : std::runtime_error(what)
    {}

    explicit ShaderParserError(const std::string& what)
        : std::runtime_error(what)
    {}
};

} // namespace snv
