//
// Created by Devilast on 08.09.2020.
//
#pragma once

#include <vector>
#include <functional>


template<typename ... Args>
class EventAction
{
public:
    EventAction() = default;

    typedef std::function<void(Args...)> handler_function;

    void Invoke(Args...);

    EventAction<Args...> operator+=(handler_function);

    EventAction<Args...> operator-=(handler_function);

    ~EventAction();

private:
    std::vector<handler_function> m_subscriptions;

    void UnsubscribeAll();

    size_t GetAddress(handler_function);
};
