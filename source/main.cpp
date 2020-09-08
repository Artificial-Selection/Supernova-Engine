//
// Created by Devilast on 08.09.2020.
//
#include <iostream>
//remove cpp include currenty testing
#include "EventAction.cpp"

int main() {
    std::cout << "project initialized" << std::endl;
    EventAction OnChange;
    OnChange.Invoke();
    std::cout << "project initialized" << std::endl;
    return 0;
}