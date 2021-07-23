#include <Input/Cursor.hpp>
#include <Core/Log.hpp>

#include <Core/Window.hpp>


namespace snv::Input
{

void Cursor::SetCursorMode(CursorMode cursorMode)
{
    if (m_currentCursorMode == cursorMode)
    {
        //LOG_WARN("Cursor mode: {}, was already set!", cursorMode);
    }
    else
    {
        m_currentCursorMode = cursorMode;
        Window::SetCursorMode(m_currentCursorMode);
    }
}

} // namespace snv::Input
