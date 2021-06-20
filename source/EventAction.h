//
// Created by Devilast on 08.09.2020.
//
#pragma once

#include <vector>
#include <functional>
#include "EngineDefinitions.h"

class EventAction
{
public:

    typedef std::function<void(int)> handler_function;

    void SNV_CALL_CONVECTION_API Invoke(int);

    EventAction SNV_CALL_CONVECTION_API operator+=(handler_function);

    EventAction SNV_CALL_CONVECTION_API operator-=(handler_function);


    ~EventAction();

private:
    std::vector<handler_function> m_subscriptions;

    void UnsubscribeAll();

    size_t GetAddress(handler_function);
};
