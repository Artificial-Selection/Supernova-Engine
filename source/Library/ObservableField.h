//
// Created by Devilast on 11.09.2020.
//
#pragma once

#include <type_traits>
#include "../EngineDefinitions.h"
#include "EventAction.h"

template<typename T>
class ObservableField
{
public:

    explicit ObservableField(T defaultValue) noexcept
    {
        _value = defaultValue;
    }
//    explicit ObservableField(int defaultValue) noexcept;

    ObservableField<T> SNV_CALL_CONVECTION_API operator+=(T value)
    {
        auto previousValue = _value;
        _value += value;
        if (previousValue != _value)
        {
            OnChange.Invoke(_value);
        }
        return *this;
    }

    ObservableField<T> SNV_CALL_CONVECTION_API operator-=(T value)
    {
        auto previousValue = _value;
        _value -= value;
        if (previousValue != _value) {
            OnChange.Invoke(_value);
        }
        return *this;
    }

private:
    T _value;
public:
    //maybe should give actual T value
    EventAction<T> OnChange;
};
