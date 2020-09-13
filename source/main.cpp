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

void OnValueChanged() {
    std::cout << "value changed" << std::endl;
}

void TestObservableField() {
    ObservableField<int> intValue(50);
    intValue.OnChange.Subscribe(OnValueChanged);
    intValue += 30;
}

int main() {
    std::cout << "project initialized" << std::endl;
    EventAction<int> OnChange;
    OnChange.Subscribe(RequestHandler);
    OnChange.Invoke(50);
    OnChange.UnsubscribeAll();
    OnChange.Invoke(30);
    TestObservableField();
    std::cout << "project ended" << std::endl;
    return 0;
}