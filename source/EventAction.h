//
// Created by Devilast on 08.09.2020.
//
#pragma once
#include <vector>
#include <functional>


template<typename ... Args>
class EventAction {
public:
    EventAction() = default;

    typedef std::function<void(Args...)> handler_function;

    void Invoke(Args...);

    void Subscribe(handler_function);

    void Unsubscribe(handler_function);

    void UnsubscribeAll();

private:
    std::vector<handler_function> m_subscriptions;
};
