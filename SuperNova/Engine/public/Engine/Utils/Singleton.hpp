//
// Created by Devilast on 6/27/2021.
//
#pragma once

template<typename T>
class Singleton
{
public:
    static T& Instance()
    {
        static T* _instance = new T();
        return *_instance;
    }

protected:
    Singleton() = default;

    ~Singleton() = default;

    Singleton(const Singleton&) = delete;

    Singleton& operator=(const Singleton&) = delete;
};
