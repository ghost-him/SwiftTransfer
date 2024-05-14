//
// Created by ghost-him on 24-4-28.
//

#include "IDGenerator.h"
#include <iostream>

uint32_t IDGenerator::getUniqueID() {
    uint32_t id = _id ++;
    return id;
}

IDGenerator::IDGenerator() {
    _id = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count() % 1000000;
    std::cerr << _id << std::endl;
}
