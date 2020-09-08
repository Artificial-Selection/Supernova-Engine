//
// Created by Devilast on 08.09.2020.
//

#include "EventAction.h"
#include <iostream>

template<typename T>
void EventAction<T>::Invoke(T *value) {
    std::cout << "invoked" << std::endl;
    std::for_each(m_subscriptions.begin(), m_subscriptions.end(), [this, value](auto element) {
        element(value);
    });
}

template<typename T>
EventAction<T> EventAction<T>::operator+=(std::function<void(void *)> handler) {
    m_subscriptions.emplace_back(handler);
    m_lastFunction = handler;
    return *this;
}

template<typename T>
EventAction<T> EventAction<T>::operator-=(std::function<void(void *)> handler) {
    //m_subscriptions.erase()
    return *this;
}
