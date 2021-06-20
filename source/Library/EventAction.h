//
// Created by Devilast on 08.09.2020.
//
#pragma once

#include <vector>
#include <iostream>
#include <algorithm>
#include <functional>

#include "../EngineDefinitions.h"

template<typename ... Args>
class EventAction
{
public:

    typedef std::function<void(Args...)> handler_function;

    void SNV_CALL_CONVECTION_API Invoke(Args... args)
    {
        std::cout << "invoked" << std::endl;
        std::for_each(m_subscriptions.begin(), m_subscriptions.end(),
                      [this, args...](auto element) { element(args...); });
    }

    EventAction<Args...> SNV_CALL_CONVECTION_API operator+=(handler_function handlerFunction)
    {
        m_subscriptions.emplace_back(handlerFunction);
        return *this;
    }

    EventAction<Args...> SNV_CALL_CONVECTION_API operator-=(handler_function handlerFunction)
    {
        auto removedElement =
                std::remove_if(m_subscriptions.begin(), m_subscriptions.end(),
                               [this, handlerAddress = GetAddress(handlerFunction)](auto function)
                               {
                                   return GetAddress(function) == handlerAddress;
                               });
        m_subscriptions.erase(removedElement, m_subscriptions.end());
        return *this;
    }

    ~EventAction()
    {
        UnsubscribeAll();
    }

private:
    std::vector<handler_function> m_subscriptions;

    void UnsubscribeAll()
    {
        m_subscriptions.clear();
    }

    size_t GetAddress(handler_function currentFunction)
    {
        typedef void(fnType)(Args...);
        fnType **fnPointer = currentFunction.template target<fnType *>();
        return (size_t) *fnPointer;
    }
};
