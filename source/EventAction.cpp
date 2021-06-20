//
// Created by Devilast on 08.09.2020.
//

#include "EventAction.h"
#include <iostream>
#include <algorithm>

void EventAction::Invoke(int arg)
{
    std::cout << "invoked" << std::endl;
    std::for_each(m_subscriptions.begin(), m_subscriptions.end(),
                  [this, arg](auto element)
                  { element(arg); });
}

void EventAction::UnsubscribeAll()
{
    m_subscriptions.clear();
}

EventAction EventAction::operator+=(handler_function handler)
{
    m_subscriptions.emplace_back(handler);
    return *this;
}

EventAction EventAction::operator-=(handler_function handler)
{
    auto removedElement =
            std::remove_if(m_subscriptions.begin(), m_subscriptions.end(),
                           [this, handlerAddress = GetAddress(handler)](auto function)
                           {
                               return GetAddress(function) == handlerAddress;
                           });
    m_subscriptions.erase(removedElement, m_subscriptions.end());
    return *this;
}


size_t EventAction::GetAddress(handler_function currentFunction)
{
    typedef void(fnType)(int);
    fnType **fnPointer = currentFunction.template target<fnType *>();
    return (size_t) *fnPointer;
}

EventAction::~EventAction()
{
    UnsubscribeAll();
}
