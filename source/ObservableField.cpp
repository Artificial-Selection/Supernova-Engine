//
// Created by Devilast on 11.09.2020.
//

#include "ObservableField.h"

template<typename T>
ObservableField<T>::ObservableField(T defaultValue) noexcept
{
    _value = defaultValue;
}

template<typename T>
ObservableField<T> ObservableField<T>::operator+=(T value)
{
    auto previousValue = _value;
    _value += value;
    if (previousValue != _value) {
        OnChange.Invoke(_value);
    }
    return *this;
}

template<typename T>
ObservableField<T> ObservableField<T>::operator-=(T value)
{
    auto previousValue = _value;
    _value -= value;
    if (previousValue != _value) {
        OnChange.Invoke(_value);
    }
    return *this;
}