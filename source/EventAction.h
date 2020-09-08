//
// Created by Devilast on 08.09.2020.
//
#include <vector>
#include <functional>


template<typename T = void>
class EventAction {
public:
    EventAction() = default;

    void Invoke(T *value = nullptr);

    EventAction<T> operator+=(std::function<void(void *)>);

    EventAction<T> operator-=(std::function<void(void *)>);

private:
    std::function<void(void *)> m_lastFunction;
    std::vector<std::function<void(void *)>> m_subscriptions;
};
