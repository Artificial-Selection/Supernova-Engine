//
// Created by Devilast on 08.09.2020.
//
#include <iostream>
#include <string>

//remove cpp include currenty testing
#include "ObservableField.cpp"
#include "EventAction.cpp"


void RequestHandler(int value) {
    std::cout << "Request proceed " << value << std::endl;
}

void OnValueChanged(int newValue) {
    std::cout << "value changed" << std::endl;
}

void TestObservableField() {
    ObservableField<int> intValue(50);
    intValue.OnChange += (OnValueChanged);
    intValue += 30;
}

void TestSecondRequest(int value) {
    std::cout << "TestSecondRequest" << std::endl;
}

int main() {
    std::cout << "project initialized" << std::endl;
    EventAction<int> OnChange;
    OnChange += RequestHandler;
    OnChange += TestSecondRequest;
    OnChange.Invoke(22);
    OnChange -= RequestHandler;
    OnChange.Invoke(22);
    TestObservableField();
    std::cout << "project ended" << std::endl;
    return 0;
}