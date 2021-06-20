//
// Created by Devilast on 08.09.2020.
//
#pragma once
#include <iostream>
#include <chrono>
#include "EventAction.h"
#include "Library/TestObservableField.h"

void RequestHandler(int value)
{
    std::cout << "Request proceed " << value << std::endl;
}

void OnValueChanged(int newValue) {
    std::cout << "value changed" << std::endl;
}

void TestObservableField()
{
    ObservableField intValue(50);
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
	TestObservableField();
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

