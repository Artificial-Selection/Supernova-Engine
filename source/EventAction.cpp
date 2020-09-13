//
// Created by Devilast on 08.09.2020.
//

#include "EventAction.h"
#include <iostream>

template<typename ... Args>
void EventAction<Args...>::Invoke(Args... args) {
    std::cout << "invoked" << std::endl;
    std::for_each(m_subscriptions.begin(), m_subscriptions.end(), [this, args...](auto element) {
        element(args...);
    });
}

template<typename ... Args>
void EventAction<Args...>::UnsubscribeAll() {
    m_subscriptions.clear();
}

template<typename ... Args>
EventAction<Args...> EventAction<Args...>::operator+=(handler_function handler) {
    m_subscriptions.emplace_back(handler);
    return *this;
}

template<typename ... Args>
EventAction<Args...> EventAction<Args...>::operator-=(handler_function handler) {
    auto removedElement = std::remove_if(m_subscriptions.begin(), m_subscriptions.end(),
                                         [this, handler](auto function) {
                                             return GetAddress(function) == GetAddress(handler);
                                         });
    m_subscriptions.erase(removedElement, m_subscriptions.end());
    return *this;
}

template<typename ... Args>
size_t EventAction<Args...>::GetAddress(handler_function currentFunction) {
    typedef void (fnType)(Args...);
    fnType **fnPointer = currentFunction.template target<fnType *>();
    return (size_t) *fnPointer;
}
