#pragma once

#include <Core/Core.hpp>


namespace snv::Input
{

enum class CursorMode : ui8
{
    Normal = 0,
    Hidden = 1,
    Locked = 2
};


class Cursor
{
public:
    static void SetCursorMode(CursorMode cursorMode);

private:
    static inline CursorMode m_currentCursorMode;
};

} // namespace snv::Input
