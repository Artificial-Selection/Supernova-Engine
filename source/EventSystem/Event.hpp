#pragma once

enum class EventType
{
    None = 0,
    KeyPressedEvent = 1,
    KeyReleasedEvent = 2
};

class Event
{
    virtual ~Event() = default;
    virtual EventType GetEventType() { return EventType::None; };
    [[maybe_unused]] static EventType GetStaticEventType() { return EventType::None; };
};