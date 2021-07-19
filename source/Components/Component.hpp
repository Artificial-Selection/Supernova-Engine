#pragma once

#include <concepts>


namespace snv
{

// NOTE(v.matushkin): Base class or traits or something else?
class BaseComponent {};

template<class T>
concept Component = std::derived_from<T, BaseComponent>;

}
