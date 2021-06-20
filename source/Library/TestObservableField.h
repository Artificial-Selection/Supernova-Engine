//
// Created by Devilast on 11.09.2020.
//
#pragma once

#include <type_traits>
#include "../EngineDefinitions.h"
#include "../EventAction.h"
//template<typename T>
//class EventAction;


class ObservableField
{
public:

    explicit ObservableField(int defaultValue) noexcept;
//    explicit ObservableField(int defaultValue) noexcept;

    int SNV_CALL_CONVECTION_API operator+=(int value);
//    ObservableField<T> SNV_CALL_CONVECTION_API operator+=(int value);

    int  SNV_CALL_CONVECTION_API operator-=(int value);
//    ObservableField<T> SNV_CALL_CONVECTION_API  operator-=(T value);

private:
    int _value;
public:
    //maybe should give actual T value
    EventAction OnChange;
};
