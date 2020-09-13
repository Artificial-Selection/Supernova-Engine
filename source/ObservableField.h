//
// Created by Devilast on 11.09.2020.
//
#pragma once

#include "EventAction.h"

template<typename T>
class ObservableField {
public:
    ObservableField() = delete;

    explicit ObservableField(T defaultValue) noexcept;

    ObservableField<T> operator+=(T value);

    ObservableField<T> operator-=(T &value);

private:
    T _value;
public:
    //maybe should give actual T value
    EventAction<> OnChange;
};
