#pragma once

#include <Engine/Core/Core.hpp>


namespace snv
{

class Time
{
public:
    // TODO(v.matushkin): This methods should be accessible only by some 'App' class
    static void Init();
    static void Update();

    static f64 GetCurrent() { return m_currentTime; }
    static f64 GetDelta()   { return m_deltaTime; }

private:
    static inline f64 m_currentTime = 0;
    static inline f64 m_deltaTime   = 0;
};

} // namespace snv
