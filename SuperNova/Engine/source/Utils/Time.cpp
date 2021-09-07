#include <Engine/Utils/Time.hpp>

#include <chrono>


using Clock    = std::chrono::high_resolution_clock;
using Duration = std::chrono::duration<f64>;


namespace snv
{

void Time::Init()
{
    m_currentTime = std::chrono::duration_cast<Duration>(Clock::now().time_since_epoch()).count();
}

void Time::Update()
{
    const f64 now = std::chrono::duration_cast<Duration>(Clock::now().time_since_epoch()).count();
    m_deltaTime = now - m_currentTime;
    m_currentTime = now;
}

} // namespace snv
