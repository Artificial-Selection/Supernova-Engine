//
// Created by Devilast on 11.09.2020.
//
#pragma once

#include <type_traits>
#include "../EngineDefinitions.h"

//template<typename T>
//class EventAction;

template<typename ...Args>
class EventAction;

template<typename T>
class ObservableField
{
public:

    explicit ObservableField(T defaultValue) noexcept;
//    explicit ObservableField(int defaultValue) noexcept;

    ObservableField<T> SNV_CALL_CONVECTION_API operator+=(T value);
//    ObservableField<T> SNV_CALL_CONVECTION_API operator+=(int value);

    ObservableField<T> SNV_CALL_CONVECTION_API operator-=(T value);
//    ObservableField<T> SNV_CALL_CONVECTION_API  operator-=(T value);

private:
    T _value;
public:
    //maybe should give actual T value
    EventAction<T> OnChange;
    EventAction<T> OnChangeInt;
};
