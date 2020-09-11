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
void EventAction<Args...>::Subscribe(handler_function handler) {
    m_subscriptions.emplace_back(handler);
}

template<typename ... Args>
void EventAction<Args...>::Unsubscribe(handler_function handler) {
    //TODO impletemnt.
}

template<typename ... Args>
void EventAction<Args...>::UnsubscribeAll() {
    m_subscriptions.clear();
}