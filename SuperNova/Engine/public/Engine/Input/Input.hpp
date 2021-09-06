#pragma once

#include <Engine/Core/Core.hpp>


namespace snv::Input
{

enum class InputAction : i8
{
    Release = 0,
    Press   = 1,
    Repeat  = 2
};

} // namespace snv::Input
