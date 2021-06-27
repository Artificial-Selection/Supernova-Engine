//
// Created by Devilast on 6/27/2021.
//

#ifndef SUPERNOVA_SINGLETON_HPP
#define SUPERNOVA_SINGLETON_HPP


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


#endif //SUPERNOVA_SINGLETON_HPP
