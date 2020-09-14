//
// Created by Devilast on 08.09.2020.
//
#include <iostream>
#include <string>
#include <chrono>
#include <thread>

//remove cpp include currenty testing
#include "ObservableField.cpp"
#include "EventAction.cpp"


void RequestHandler(int value)
{
    std::cout << "Request proceed " << value << std::endl;
}

void OnValueChanged(int newValue) {
    std::cout << "value changed" << std::endl;
}

void TestObservableField()
{
    ObservableField<int> intValue(50);
    intValue.OnChange += OnValueChanged;
    intValue += 30;
}

void TestSecondRequest(int value)
{
    std::cout << "TestSecondRequest" << std::endl;
}

void ProcessInput()
{
    //TODO IMPLEMENT
}

void Render()
{
    //TODO Implement
}

void Update(float deltaTime)
{
    //TODO Implement
}

int main()
{
    std::cout << "project initialized" << std::endl;
    const int maxFPS = 60;
    const int maxPeriod = 1.0 / maxFPS;
    auto startTime = std::chrono::high_resolution_clock::now().time_since_epoch();
    while (true) {
        //TODO FPS lock
        auto currentTime = std::chrono::high_resolution_clock::now().time_since_epoch();
        auto elapsed = 1.0 / (currentTime - startTime).count();
        std::cout << "current lag is " << elapsed << std::endl;
        startTime = currentTime;
        ProcessInput();
        Update(elapsed);
        Render();
    }
    std::cout << "project ended" << std::endl;
    return 0;
}

